#include <format>
#include <fstream>
#include <string>

#include "Events.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"

AnimationManager& AnimationManager::GetSingleton() {
    static AnimationManager instance;
    return instance;
}

// --- Lógica de Escaneamento (Carrega a Biblioteca) ---
void AnimationManager::ScanAnimationMods() {
    SKSE::log::info("Iniciando escaneamento da biblioteca de animações...");
    _categories.clear();
    _allMods.clear();
    const std::filesystem::path oarRootPath = "Data\\meshes\\actors\\character\\animations\\OpenAnimationReplacer";
    std::map<double, std::string> typeToName = {{1.0, "Swords"},     {2.0, "Daggers"},     {3.0, "War Axes"},
                                                {4.0, "Maces"},      {5.0, "Greatswords"}, {6.0, "Bows"},
                                                {7.0, "Battleaxes"}, {8.0, "Warhammers"}};
    for (const auto& pair : typeToName) {
        _categories[pair.second].name = pair.second;
        _categories[pair.second].equippedTypeValue = pair.first;
    }
    if (!std::filesystem::exists(oarRootPath)) return;
    for (const auto& entry : std::filesystem::directory_iterator(oarRootPath)) {
        if (entry.is_directory()) {
            ProcessTopLevelMod(entry.path());
        }
    }
    SKSE::log::info("Escaneamento finalizado. {} mods carregados na biblioteca.", _allMods.size());
}

void AnimationManager::ProcessTopLevelMod(const std::filesystem::path& modPath) {
    std::filesystem::path configPath = modPath / "config.json";
    if (!std::filesystem::exists(configPath)) return;
    std::ifstream fileStream(configPath);
    std::string jsonContent((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
    fileStream.close();
    rapidjson::Document doc;
    doc.Parse(jsonContent.c_str());
    if (doc.IsObject() && doc.HasMember("name") && doc.HasMember("author")) {
        AnimationModDef modDef;
        modDef.name = doc["name"].GetString();
        modDef.author = doc["author"].GetString();
        for (const auto& subEntry : std::filesystem::recursive_directory_iterator(modPath)) {
            if (subEntry.is_directory() && std::filesystem::exists(subEntry.path() / "config.json")) {
                if (std::filesystem::equivalent(modPath, subEntry.path())) continue;
                SubAnimationDef subAnimDef;
                subAnimDef.name = subEntry.path().filename().string();
                subAnimDef.path = subEntry.path() / "config.json";
                modDef.subAnimations.push_back(subAnimDef);
            }
        }
        _allMods.push_back(modDef);
    }
}

// --- Lógica da Interface de Usuário ---
void AnimationManager::DrawAddModModal() {
    if (_isAddModModalOpen) {
        if (_instanceToAddTo)
            ImGui::OpenPopup("Adicionar Moveset");
        else if (_modInstanceToAddTo)
            ImGui::OpenPopup("Adicionar Sub-Moveset");
        _isAddModModalOpen = false;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 center = ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Adicionar Moveset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Biblioteca de Movesets");
        ImGui::Separator();
        if (ImGui::BeginChild("BibliotecaMovesets", ImVec2(600, 400), true)) {
            for (size_t modIdx = 0; modIdx < _allMods.size(); ++modIdx) {
                const auto& modDef = _allMods[modIdx];
                ImGui::Text("%s", modDef.name.c_str());
                ImGui::SameLine(ImGui::GetWindowWidth() - 100);
                if (ImGui::Button(("Adicionar##" + modDef.name).c_str())) {
                    ModInstance newModInstance;
                    newModInstance.sourceModIndex = modIdx;
                    for (size_t subIdx = 0; subIdx < modDef.subAnimations.size(); ++subIdx) {
                        SubAnimationInstance newSubInstance;
                        newSubInstance.sourceModIndex = modIdx;
                        newSubInstance.sourceSubAnimIndex = subIdx;
                        newModInstance.subAnimationInstances.push_back(newSubInstance);
                    }
                    _instanceToAddTo->modInstances.push_back(newModInstance);
                }
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Fechar")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Adicionar Sub-Moveset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Biblioteca de Animações");
        ImGui::Separator();
        if (ImGui::BeginChild("BibliotecaSubMovesets", ImVec2(600, 400), true)) {
            for (size_t modIdx = 0; modIdx < _allMods.size(); ++modIdx) {
                const auto& modDef = _allMods[modIdx];
                if (ImGui::TreeNode(modDef.name.c_str())) {
                    for (size_t subAnimIdx = 0; subAnimIdx < modDef.subAnimations.size(); ++subAnimIdx) {
                        const auto& subAnimDef = modDef.subAnimations[subAnimIdx];
                        ImGui::Text("%s", subAnimDef.name.c_str());
                        ImGui::SameLine();
                        ImGui::PushID(static_cast<int>(modIdx * 1000 + subAnimIdx));
                        if (ImGui::Button("Adicionar")) {
                            SubAnimationInstance newSubInstance;
                            newSubInstance.sourceModIndex = modIdx;
                            newSubInstance.sourceSubAnimIndex = subAnimIdx;
                            _modInstanceToAddTo->subAnimationInstances.push_back(newSubInstance);
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Fechar")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void AnimationManager::DrawImGuiMenu() {
    if (ImGui::Button("Salvar Todas as Configurações")) {
        // SaveAllSettings();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Preservar Condições Externas", &_preserveConditions);
    ImGui::Separator();

    DrawAddModModal();

    if (_categories.empty()) {
        ImGui::Text("Nenhuma categoria de animação foi carregada.");
        return;
    }

    // Loop principal para desenhar cada categoria de arma
    for (auto& pair : _categories) {
        WeaponCategory& category = pair.second;
        ImGui::PushID(category.name.c_str());
        // CORREÇÃO: Usando a lógica simples que sempre funciona, sem acordeão por enquanto.
        if (ImGui::CollapsingHeader(category.name.c_str())) {
            
            if (ImGui::BeginTabBar("InstanceTabs")) {
                for (int i = 0; i < 4; ++i) {
                    if (ImGui::BeginTabItem(std::format("Instância {}", i + 1).c_str())) {
                        category.activeInstanceIndex = i;
                        CategoryInstance& instance = category.instances[i];

                        // Botões de ação para a instância
                        if (ImGui::Button("Adicionar Moveset")) {
                            _isAddModModalOpen = true;
                            _instanceToAddTo = &instance;
                            _modInstanceToAddTo = nullptr;
                        }
                        ImGui::Separator();

                        int modInstanceToRemove = -1;
                        int playlistEntryCounter = 1;

                        // Loop para os Movesets (ModInstance)
                        for (size_t mod_i = 0; mod_i < instance.modInstances.size(); ++mod_i) {
                            auto& modInstance = instance.modInstances[mod_i];
                            const auto& sourceMod = _allMods[modInstance.sourceModIndex];

                            ImGui::PushID(static_cast<int>(mod_i));
                            if (ImGui::Button("X")) modInstanceToRemove = static_cast<int>(mod_i);
                            ImGui::SameLine();
                            ImGui::Checkbox("##modselect", &modInstance.isSelected);
                            ImGui::SameLine();

                            bool node_open = ImGui::TreeNode(sourceMod.name.c_str());

                            // Drag and Drop para MOVESETS
                            if (ImGui::BeginDragDropSource()) {
                                ImGui::SetDragDropPayload("DND_MOD_INSTANCE", &mod_i, sizeof(size_t));
                                ImGui::Text("Mover moveset %s", sourceMod.name.c_str());
                                ImGui::EndDragDropSource();
                            }
                            if (ImGui::BeginDragDropTarget()) {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_MOD_INSTANCE")) {
                                    size_t source_idx = *(const size_t*)payload->Data;
                                    std::swap(instance.modInstances[source_idx], instance.modInstances[mod_i]);
                                }
                            }

                            if (node_open) {
                                if (ImGui::Button("Adicionar Sub-Moveset")) {
                                    _isAddModModalOpen = true;
                                    _modInstanceToAddTo = &modInstance;
                                    _instanceToAddTo = nullptr;
                                }

                                // Loop para os Sub-Movesets (SubAnimationInstance)
                                for (size_t sub_j = 0; sub_j < modInstance.subAnimationInstances.size(); ++sub_j) {
                                    auto& subInstance = modInstance.subAnimationInstances[sub_j];
                                    const auto& originMod = _allMods[subInstance.sourceModIndex];
                                    const auto& originSubAnim = originMod.subAnimations[subInstance.sourceSubAnimIndex];

                                    ImGui::PushID(static_cast<int>(sub_j));
                                    ImGui::Checkbox("##subselect", &subInstance.isSelected);
                                    ImGui::SameLine();

                                    std::string label;
                                    if (subInstance.isSelected) {
                                        label = std::format("[{}] {}", playlistEntryCounter++, originSubAnim.name);
                                    } else {
                                        label = std::format("    {}", originSubAnim.name);
                                    }

                                    if (subInstance.sourceModIndex != modInstance.sourceModIndex) {
                                        label += std::format(" (by: {})", originMod.name);
                                    }

                                    ImVec2 regionAvail;
                                    ImGui::GetContentRegionAvail(&regionAvail);
                                    ImGui::Selectable(label.c_str(), false, 0,
                                                      ImVec2(regionAvail.x - 450, 0));

                                    // Drag and Drop para SUB-MOVESETS
                                    if (ImGui::BeginDragDropSource()) {
                                        ImGui::SetDragDropPayload("DND_SUB_INSTANCE", &sub_j, sizeof(size_t));
                                        ImGui::Text("Mover %s", originSubAnim.name.c_str());
                                        ImGui::EndDragDropSource();
                                    }
                                    if (ImGui::BeginDragDropTarget()) {
                                        if (const ImGuiPayload* payload =
                                                ImGui::AcceptDragDropPayload("DND_SUB_INSTANCE")) {
                                            size_t source_idx = *(const size_t*)payload->Data;
                                            std::swap(modInstance.subAnimationInstances[source_idx],
                                                      modInstance.subAnimationInstances[sub_j]);
                                        }
                                    }

                                    ImGui::SameLine();
                                    ImGui::Checkbox("F", &subInstance.pFront);
                                    ImGui::SameLine();
                                    ImGui::Checkbox("B", &subInstance.pBack);
                                    ImGui::SameLine();
                                    ImGui::Checkbox("L", &subInstance.pLeft);
                                    ImGui::SameLine();
                                    ImGui::Checkbox("R", &subInstance.pRight);
                                    ImGui::SameLine();
                                    ImGui::Checkbox("FR", &subInstance.pFrontRight);
                                    ImGui::SameLine();
                                    ImGui::Checkbox("FL", &subInstance.pFrontLeft);
                                    ImGui::SameLine();
                                    ImGui::Checkbox("BR", &subInstance.pBackRight);
                                    ImGui::SameLine();
                                    ImGui::Checkbox("BL", &subInstance.pBackLeft);
                                    ImGui::SameLine();
                                    ImGui::Checkbox("Rnd", &subInstance.pRandom);

                                    ImGui::PopID();
                                }
                                ImGui::TreePop();
                            }
                            ImGui::PopID();
                        }

                        if (modInstanceToRemove != -1) {
                            instance.modInstances.erase(instance.modInstances.begin() + modInstanceToRemove);
                        }
                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::PopID();
        
    }
}

// --- Funções Vazias (Serão implementadas a seguir) ---
void AnimationManager::SaveAllSettings() { SKSE::log::info("Função de salvamento ainda não implementada."); }
void AnimationManager::UpdateOrCreateJson(const std::filesystem::path&, const std::vector<FileSaveConfig>&) {}
void AnimationManager::AddCompareValuesCondition(rapidjson::Value&, const std::string&, int,
                                                 rapidjson::Document::AllocatorType&) {}
