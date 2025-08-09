#include <format>
#include <fstream>
#include <string>
#include <algorithm>
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
    // ESTRUTURA MELHORADA: Facilita a definição de categorias e suas propriedades
    struct CategoryDefinition {
        std::string name;
        double typeValue;
        bool isDual;
    };

    std::vector<CategoryDefinition> categoryDefinitions = {{"Swords", 1.0, false},
                                                           {"Daggers", 2.0, false},
                                                           {"War Axes", 3.0, false},
                                                           {"Maces", 4.0, false},
                                                           {"Greatswords", 5.0, false},
                                                           {"Bows", 6.0, false},
                                                           {"Battleaxes", 7.0, false},
                                                           {"Warhammers", 8.0, false},
                                                           // NOVAS CATEGORIAS DUAL WIELD
                                                           {"Dual Swords", 1.0, true},
                                                           {"Dual Daggers", 2.0, true},
                                                           {"Dual War Axes", 3.0, true},
                                                           {"Dual Maces", 4.0, true}};

    for (const auto& def : categoryDefinitions) {
        _categories[def.name].name = def.name;
        _categories[def.name].equippedTypeValue = def.typeValue;
        _categories[def.name].isDualWield = def.isDual;
    }

    if (!std::filesystem::exists(oarRootPath)) return;
    for (const auto& entry : std::filesystem::directory_iterator(oarRootPath)) {
        if (entry.is_directory()) {
            ProcessTopLevelMod(entry.path());
        }
    }
    SKSE::log::info("Escaneamento de arquivos finalizado. {} mods carregados.", _allMods.size());

    // --- NOVA SEÇÃO: Carregar e integrar movesets do usuário ---
    LoadUserMovesets();

    for (const auto& userMoveset : _userMovesets) {
        AnimationModDef modDef;
        modDef.name = userMoveset.name;
        modDef.author = "Usuário";  // Autor padrão

        for (const auto& subInstance : userMoveset.subAnimations) {
            // Verifica se os índices são válidos para evitar crashes
            if (subInstance.sourceModIndex < _allMods.size()) {
                const auto& sourceMod = _allMods[subInstance.sourceModIndex];
                if (subInstance.sourceSubAnimIndex < sourceMod.subAnimations.size()) {
                    // Adiciona a definição da sub-animação original ao nosso novo mod virtual
                    modDef.subAnimations.push_back(sourceMod.subAnimations[subInstance.sourceSubAnimIndex]);
                }
            }
        }
        _allMods.push_back(modDef);
    }
    SKSE::log::info("Integração finalizada. Total de {} mods na biblioteca (incluindo de usuário).", _allMods.size());
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


//void AnimationManager::LoadStateForSubAnimation(size_t modIdx, size_t subAnimIdx) {
//    const auto& subAnimDef = _allMods[modIdx].subAnimations[subAnimIdx];
//    const auto& jsonPath = subAnimDef.path;
//
//    if (!std::filesystem::exists(jsonPath)) return;
//
//    std::ifstream fileStream(jsonPath);
//    std::string jsonContent((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
//    fileStream.close();
//
//    rapidjson::Document doc;
//    if (doc.Parse(jsonContent.c_str()).HasParseError() || !doc.IsObject() || !doc.HasMember("conditions")) {
//        return;
//    }
//
//    const rapidjson::Value& conditions = doc["conditions"];
//    if (!conditions.IsArray()) return;
//
//    // Encontra o nosso bloco de condições gerenciado
//    for (const auto& topCond : conditions.GetArray()) {
//        if (topCond.IsObject() && topCond.HasMember("comment") &&
//            strcmp(topCond["comment"].GetString(), "OAR_CYCLE_MANAGER_CONDITIONS") == 0 &&
//            topCond.HasMember("Conditions") && topCond["Conditions"].IsArray()) {
//            const rapidjson::Value& andBlocks = topCond["Conditions"];
//            // Cada bloco AND corresponde a uma configuração salva para esta animação
//            for (const auto& andBlock : andBlocks.GetArray()) {
//                if (!andBlock.IsObject() || !andBlock.HasMember("Conditions") || !andBlock["Conditions"].IsArray())
//                    continue;
//
//                // Extrai os dados do bloco
//                double equippedTypeValue = 0.0;
//                bool isDual = false;
//                int instance_index = 0;
//                float directionalValue = 0.0;
//                int randomValue = 0;
//
//                const rapidjson::Value& finalConditions = andBlock["Conditions"];
//                for (const auto& cond : finalConditions.GetArray()) {
//                    if (!cond.IsObject() || !cond.HasMember("condition")) continue;
//                    std::string condName = cond["condition"].GetString();
//
//                    if (condName == "IsEquippedType" && cond.HasMember("Type")) {
//                        equippedTypeValue = cond["Type"]["value"].GetDouble();
//                        if (cond.HasMember("Left hand") && cond["Left hand"].GetBool()) {
//                            isDual = true;
//                        }
//                    }
//                    if (condName == "CompareValues" && cond.HasMember("Value B")) {
//                        std::string varName = cond["Value B"]["graphVariable"].GetString();
//                        if (varName == "cycle_instance") instance_index = cond["Value A"]["value"].GetInt();
//                        if (varName == "DirecionalCycleMoveset") directionalValue = cond["Value A"]["value"].GetFloat();
//                    }
//                    if (condName == "Random") {
//                        if (cond.HasMember("Minimum random value"))
//                            randomValue = cond["Minimum random value"]["value"].GetInt();
//                    }
//                }
//
//                // Se não encontramos dados válidos, pulamos
//                if (instance_index == 0) continue;
//
//                // Encontra a categoria correta
//                WeaponCategory* targetCategory = nullptr;
//                for (auto& pair : _categories) {
//                    if (pair.second.equippedTypeValue == equippedTypeValue && pair.second.isDualWield == isDual) {
//                        targetCategory = &pair.second;
//                        break;
//                    }
//                }
//                if (!targetCategory) continue;
//                // Se o índice da instância que queremos acessar não existe ainda no vetor...
//                if (instance_index < 1 || instance_index > 4) {
//                    // Se o índice for inválido (ex: 0, 5, etc.), pula para a próxima iteração do loop.
//                    SKSE::log::warn("Índice de instância inválido ({}) encontrado no JSON. Ignorando.",
//                                    instance_index);  // Log opcional para debug
//                    continue;
//                }
//                // Cria a instância da sub-animação com os dados carregados
//                SubAnimationInstance newSubInstance;
//                newSubInstance.sourceModIndex = modIdx;
//                newSubInstance.sourceSubAnimIndex = subAnimIdx;
//                newSubInstance.isSelected = true;  // Se está salva, está selecionada
//
//                if (randomValue > 0) {
//                    newSubInstance.pRandom = true;
//                } else if (directionalValue > 0.0f) {
//                    if (directionalValue == 1.0f)
//                        newSubInstance.pFront = true;
//                    else if (directionalValue == 2.0f)
//                        newSubInstance.pFrontRight = true;
//                    else if (directionalValue == 3.0f)
//                        newSubInstance.pRight = true;
//                    else if (directionalValue == 4.0f)
//                        newSubInstance.pBackRight = true;
//                    else if (directionalValue == 5.0f)
//                        newSubInstance.pBack = true;
//                    else if (directionalValue == 6.0f)
//                        newSubInstance.pBackLeft = true;
//                    else if (directionalValue == 7.0f)
//                        newSubInstance.pLeft = true;
//                    else if (directionalValue == 8.0f)
//                        newSubInstance.pFrontLeft = true;
//                }
//
//                // Cria um ModInstance "pai" para esta sub-animação
//                ModInstance newModInstance;
//                newModInstance.sourceModIndex = modIdx;
//                newModInstance.isSelected = true;
//                newModInstance.subAnimationInstances.push_back(newSubInstance);
//
//                // Adiciona na instância da categoria correta
//                targetCategory->instances[instance_index - 1].modInstances.push_back(newModInstance);
//            }
//            break;
//        }
//    }
//}


// --- Lógica da Interface de Usuário ---
void AnimationManager::DrawAddModModal() {
    if (_isAddModModalOpen) {
        if (_instanceToAddTo) {
            ImGui::OpenPopup("Adicionar Moveset");
        }
        // CORREÇÃO: O modal de sub-moveset agora abre se QUALQUER um dos ponteiros for válido.
        else if (_modInstanceToAddTo || _userMovesetToAddTo) {
            ImGui::OpenPopup("Adicionar Sub-Moveset");
        }
        _isAddModModalOpen = false;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 center = ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Adicionar Moveset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Biblioteca de Movesets");
        ImGui::Separator();
        // NOVO: Caixa de pesquisa para movesets
        ImGui::InputText("Filtrar", _movesetFilter, 128);
        if (ImGui::BeginChild("BibliotecaMovesets", ImVec2(600, 400), true)) {
            // Prepara o filtro para busca case-insensitive
            std::string filter_str = _movesetFilter;
            std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

            for (size_t modIdx = 0; modIdx < _allMods.size(); ++modIdx) {
                const auto& modDef = _allMods[modIdx];

                std::string mod_name_str = modDef.name;
                std::transform(mod_name_str.begin(), mod_name_str.end(), mod_name_str.begin(), ::tolower);

                // Aplica o filtro
                if (filter_str.empty() || mod_name_str.find(filter_str) != std::string::npos) {
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
        }
        ImGui::EndChild();
        if (ImGui::Button("Fechar")) {
            strcpy_s(_movesetFilter, "");  // Limpa o filtro ao fechar
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Adicionar Sub-Moveset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Biblioteca de Animações");
        ImGui::Separator();
        // NOVO: Caixa de pesquisa para sub-movesets
        ImGui::InputText("Filtrar", _subMovesetFilter, 128);

        if (ImGui::BeginChild("BibliotecaSubMovesets", ImVec2(600, 400), true)) {
            std::string filter_str = _subMovesetFilter;
            std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

            for (size_t modIdx = 0; modIdx < _allMods.size(); ++modIdx) {
                const auto& modDef = _allMods[modIdx];

                std::string mod_name_str = modDef.name;
                std::transform(mod_name_str.begin(), mod_name_str.end(), mod_name_str.begin(), ::tolower);
                bool parent_matches = mod_name_str.find(filter_str) != std::string::npos;

                // Verifica se algum filho corresponde ao filtro
                bool child_matches = false;
                if (!parent_matches) {
                    for (const auto& subAnim : modDef.subAnimations) {
                        std::string sub_name_str = subAnim.name;
                        std::transform(sub_name_str.begin(), sub_name_str.end(), sub_name_str.begin(), ::tolower);
                        if (sub_name_str.find(filter_str) != std::string::npos) {
                            child_matches = true;
                            break;
                        }
                    }
                }

                // Mostra o TreeNode se o filtro estiver vazio, ou se o pai ou algum filho corresponder
                if (filter_str.empty() || parent_matches || child_matches) {
                    if (ImGui::TreeNode(modDef.name.c_str())) {
                        for (size_t subAnimIdx = 0; subAnimIdx < modDef.subAnimations.size(); ++subAnimIdx) {
                            const auto& subAnimDef = modDef.subAnimations[subAnimIdx];

                            std::string sub_name_str = subAnimDef.name;
                            std::transform(sub_name_str.begin(), sub_name_str.end(), sub_name_str.begin(), ::tolower);

                            // Mostra o filho se o filtro estiver vazio ou se ele corresponder
                            if (filter_str.empty() || sub_name_str.find(filter_str) != std::string::npos) {
                                ImGui::Text("%s", subAnimDef.name.c_str());
                                ImGui::SameLine();
                                ImGui::PushID(static_cast<int>(modIdx * 1000 + subAnimIdx));
                                if (ImGui::Button("Adicionar")) {
                                    SubAnimationInstance newSubInstance;
                                    newSubInstance.sourceModIndex = modIdx;
                                    newSubInstance.sourceSubAnimIndex = subAnimIdx;
                                    if (_modInstanceToAddTo) {  // Se estamos adicionando a um ModInstance (sistema
                                                                // antigo)
                                        _modInstanceToAddTo->subAnimationInstances.push_back(newSubInstance);
                                    } else if (_userMovesetToAddTo) {  // Se estamos adicionando a um UserMoveset
                                                                       // (sistema novo)
                                        _userMovesetToAddTo->subAnimations.push_back(newSubInstance);
                                    }
                                }
                            }
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

// Esta é a nova função principal da UI que você registrará no SKSEMenuFramework
void AnimationManager::DrawMainMenu() {
    // Primeiro, desenhamos o sistema de abas
    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Gerenciador de Animações")) {
            DrawAnimationManager();  // Chama a UI da primeira aba
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Gerenciar Meus Movesets")) {
            DrawUserMovesetManager();  // Chama a UI da segunda aba
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // CORREÇÃO: Chamamos a função do modal aqui, fora de qualquer aba.
    // Ele só será desenhado quando a flag _isAddModModalOpen for verdadeira,
    // mas agora ele não pertence a nenhuma aba específica.
    DrawAddModModal();
}

void AnimationManager::DrawAnimationManager() {
    if (ImGui::Button("Save config")) {
        SaveAllSettings();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Preservar Condições Externas", &_preserveConditions);
    ImGui::Separator();

    // DrawAddModModal();

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
            
            if (ImGui::BeginTabBar("StanceTabs")) {
                for (int i = 0; i < 4; ++i) {
                    if (ImGui::BeginTabItem(std::format("Stance {}", i + 1).c_str())) {
                        category.activeInstanceIndex = i;
                        CategoryInstance& instance = category.instances[i];

                        std::map<SubAnimationInstance*, int> playlistNumbers;
                        std::map<SubAnimationInstance*, int> parentNumbersForChildren;
                        int currentPlaylistCounter = 1;
                        int lastValidParentNumber = 0;

                        for (auto& modInst : instance.modInstances) {
                            if (!modInst.isSelected) continue;  // Pula movesets desativados

                            for (auto& subInst : modInst.subAnimationInstances) {
                                if (!subInst.isSelected) continue;  // Pula sub-movesets desativados

                                // Sua lógica para determinar se é um "Pai"
                                bool isParent = !(subInst.pFront || subInst.pBack || subInst.pLeft || subInst.pRight ||
                                                  subInst.pFrontRight || subInst.pFrontLeft || subInst.pBackRight ||
                                                  subInst.pBackLeft || subInst.pRandom || subInst.pDodge);

                                if (isParent) {
                                    lastValidParentNumber = currentPlaylistCounter;
                                    playlistNumbers[&subInst] = currentPlaylistCounter;
                                    currentPlaylistCounter++;
                                } else {
                                    // É um filho, armazena o número do seu pai
                                    parentNumbersForChildren[&subInst] = lastValidParentNumber;
                                }
                            }
                        }

                        // Botões de ação para a instância
                        if (ImGui::Button("Adicionar Moveset")) {
                            _isAddModModalOpen = true;
                            _instanceToAddTo = &instance;
                            _modInstanceToAddTo = nullptr;
                        }
                        ImGui::Separator();

                        int modInstanceToRemove = -1;
                        int playlistEntryCounter = 1;  // Contador apenas para "Pais"
                        // Loop para os Movesets (ModInstance)
                        for (size_t mod_i = 0; mod_i < instance.modInstances.size(); ++mod_i) {
                            auto& modInstance = instance.modInstances[mod_i];
                            const auto& sourceMod = _allMods[modInstance.sourceModIndex];

                            ImGui::PushID(static_cast<int>(mod_i));
                            // --- INÍCIO DA CORREÇÃO DO BUG VISUAL ---

                            // 1. Salvamos o estado ANTES de desenhar qualquer coisa.
                            const bool isParentDisabled = !modInstance.isSelected;

                            // 2. Aplicamos a cor cinza se o estado for "desabilitado".
                            if (isParentDisabled) {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle()->Colors[ImGuiCol_TextDisabled]);
                            }

                            // 3. Desenhamos todos os widgets do "Pai" (botão, checkbox, nome)
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
                                // Estas variáveis agora controlam a lógica de agrupamento

                                int lastParentNumber = 0;   // Armazena o número do último "Pai"
                                // NOVO: Variável para marcar um sub-moveset para remoção
                                //int subInstanceToRemove = -1;

                                // Loop para os Sub-Movesets (SubAnimationInstance)
                                for (size_t sub_j = 0; sub_j < modInstance.subAnimationInstances.size(); ++sub_j) {
                                    auto& subInstance = modInstance.subAnimationInstances[sub_j];
                                    const auto& originMod = _allMods[subInstance.sourceModIndex];
                                    const auto& originSubAnim = originMod.subAnimations[subInstance.sourceSubAnimIndex];

                                    ImGui::PushID(static_cast<int>(sub_j));
                                    // A cor do filho depende do seu próprio estado ou do estado do pai.
                                    const bool isChildDisabled = !subInstance.isSelected || isParentDisabled;

                                    if (isChildDisabled) {
                                        ImGui::PushStyleColor(ImGuiCol_Text,
                                                              ImGui::GetStyle()->Colors[ImGuiCol_TextDisabled]);
                                    }

                                    ImGui::Checkbox("##subselect", &subInstance.isSelected);
                                    ImGui::SameLine();


                                    std::string label;

                                    // A lógica de contagem foi substituída pela busca no map
                                    if (modInstance.isSelected && subInstance.isSelected) {
                                        if (playlistNumbers.count(&subInstance)) {  // É um "Pai" com número calculado
                                            label = std::format("[{}] {}", playlistNumbers.at(&subInstance),
                                                                originSubAnim.name);
                                        } else if (parentNumbersForChildren.count(&subInstance)) {  // É um "Filho"
                                            int parentNum = parentNumbersForChildren.at(&subInstance);
                                            label = std::format(" -> [{}] {}", parentNum, originSubAnim.name);
                                        } else {  // Caso não esteja na contagem (é filho sem pai ou algo inesperado)
                                            label = originSubAnim.name;
                                        }
                                    } else {
                                        label = originSubAnim.name;  // Desativado, mostra sem número
                                    }

                                    // Adiciona o "by: author" se for de um mod diferente
                                    if (subInstance.sourceModIndex != modInstance.sourceModIndex) {
                                        label += std::format(" (by: {})", originMod.name);
                                    }

                                    ImVec2 regionAvail;
                                    ImGui::GetContentRegionAvail(&regionAvail);
                                    ImGui::Selectable(label.c_str(), false, 0,
                                                      ImVec2(regionAvail.x - 800, 0));


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
                                    ImGui::SameLine();                          
                                    ImGui::Checkbox("Dodge", &subInstance.pDodge);  

                                    if (isChildDisabled) {
                                        ImGui::PopStyleColor();
                                    }

                                    ImGui::PopID();
                                }

                                ImGui::TreePop();

                            }  
                            if (isParentDisabled) {
                                ImGui::PopStyleColor();
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


void AnimationManager::SaveAllSettings() {
    SKSE::log::info("Iniciando salvamento global de todas as configurações...");
    std::map<std::filesystem::path, std::vector<FileSaveConfig>> fileUpdates;

    // 1. Loop através de cada CATEGORIA de arma
    for (auto& pair : _categories) {
        WeaponCategory& category = pair.second;

        // 2. Loop através de cada uma das 4 INSTÂNCIAS
        for (int i = 0; i < 4; ++i) {
            CategoryInstance& instance = category.instances[i];

            // 3. Loop através dos MOVESETS (ModInstance) na instância
            for (size_t mod_i = 0; mod_i < instance.modInstances.size(); ++mod_i) {
                ModInstance& modInstance = instance.modInstances[mod_i];

                int playlistParentCounter = 1;  // Contador para os itens "Pai"
                int lastParentOrder = 0;        // Armazena o número do último "Pai"

                // 4. Loop através dos SUB-MOVESETS (SubAnimationInstance)
                for (size_t sub_j = 0; sub_j < modInstance.subAnimationInstances.size(); ++sub_j) {
                    SubAnimationInstance& subInstance = modInstance.subAnimationInstances[sub_j];

                    // Salva apenas se tanto o sub-moveset quanto o moveset pai estiverem selecionados
                    if (modInstance.isSelected && subInstance.isSelected) {
                        const auto& sourceMod = _allMods[subInstance.sourceModIndex];
                        const auto& sourceSubAnim = sourceMod.subAnimations[subInstance.sourceSubAnimIndex];

                        FileSaveConfig config;
                        config.instance_index = i + 1;  // Instância é 1-4
                        config.category = &category;

                        // Copia o estado de todas as checkboxes para o config
                        config.pFront = subInstance.pFront;
                        config.pBack = subInstance.pBack;
                        config.pLeft = subInstance.pLeft;
                        config.pRight = subInstance.pRight;
                        config.pFrontRight = subInstance.pFrontRight;
                        config.pFrontLeft = subInstance.pFrontLeft;
                        config.pBackRight = subInstance.pBackRight;
                        config.pBackLeft = subInstance.pBackLeft;
                        config.pRandom = subInstance.pRandom;


                        // Determina se é um "Pai" (nenhuma checkbox de direção marcada) ou "Filho"
                        bool isParent = !(config.pFront || config.pBack || config.pLeft || config.pRight ||
                                          config.pFrontRight || config.pFrontLeft || config.pBackRight ||
                                          config.pBackLeft || config.pRandom || config.pDodge);

                        if (isParent) {
                            lastParentOrder = playlistParentCounter;
                            config.order_in_playlist = playlistParentCounter++;
                        } else {
                            // Filhos herdam o número do último pai encontrado
                            config.order_in_playlist = lastParentOrder;
                        }

                        // Adiciona a configuração ao mapa, agrupada pelo caminho do arquivo
                        fileUpdates[sourceSubAnim.path].push_back(config);
                    }
                }
            }
        }
    }

    SKSE::log::info("{} arquivos de configuração serão modificados.", fileUpdates.size());
    for (const auto& updateEntry : fileUpdates) {
        UpdateOrCreateJson(updateEntry.first, updateEntry.second);
    }

    SKSE::log::info("Salvamento global concluído.");
    RE::DebugNotification("Todas as configurações foram salvas!");
}



void AnimationManager::UpdateOrCreateJson(const std::filesystem::path& jsonPath,
                                          const std::vector<FileSaveConfig>& configs) {
    rapidjson::Document doc;
    std::ifstream fileStream(jsonPath);
    if (fileStream) {
        std::string jsonContent((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
        fileStream.close();
        if (doc.Parse(jsonContent.c_str()).HasParseError()) {
            SKSE::log::error("Erro de Parse ao ler {}. Criando um novo arquivo.", jsonPath.string());
            doc.SetObject();
        }
    } else {
        doc.SetObject();
    }

    if (!doc.IsObject()) doc.SetObject();
    auto& allocator = doc.GetAllocator();

    rapidjson::Value oldConditions(rapidjson::kArrayType);
    if (_preserveConditions && doc.HasMember("conditions") && doc["conditions"].IsArray()) {
        for (const auto& cond : doc["conditions"].GetArray()) {
            rapidjson::Value c;
            c.CopyFrom(cond, allocator);
            oldConditions.PushBack(c, allocator);
        }
    }

    if (doc.HasMember("conditions")) {
        doc["conditions"].SetArray();
    } else {
        doc.AddMember("conditions", rapidjson::Value(rapidjson::kArrayType), allocator);
    }
    rapidjson::Value& conditions = doc["conditions"];

    if (_preserveConditions && !oldConditions.Empty()) {
        rapidjson::Value oldConditionsBlock(rapidjson::kObjectType);
        oldConditionsBlock.AddMember("condition", "OR", allocator);
        oldConditionsBlock.AddMember("comment", "Old Conditions", allocator);
        oldConditionsBlock.AddMember("Conditions", oldConditions, allocator);
        conditions.PushBack(oldConditionsBlock, allocator);
    }

    rapidjson::Value masterOrBlock(rapidjson::kObjectType);
    masterOrBlock.AddMember("condition", "OR", allocator);
    masterOrBlock.AddMember("comment", "OAR_CYCLE_MANAGER_CONDITIONS", allocator);
    rapidjson::Value innerConditions(rapidjson::kArrayType);

    for (const auto& config : configs) {
        rapidjson::Value categoryAndBlock(rapidjson::kObjectType);
        categoryAndBlock.AddMember("condition", "AND", allocator);
        rapidjson::Value andConditions(rapidjson::kArrayType);

        {
            rapidjson::Value actorBase(rapidjson::kObjectType);
            actorBase.AddMember("condition", "IsActorBase", allocator);
            rapidjson::Value actorBaseParams(rapidjson::kObjectType);
            actorBaseParams.AddMember("pluginName", "Skyrim.esm", allocator);
            actorBaseParams.AddMember("formID", "7", allocator);
            actorBase.AddMember("Actor base", actorBaseParams, allocator);
            andConditions.PushBack(actorBase, allocator);
        }
        {
            rapidjson::Value equippedType(rapidjson::kObjectType);
            equippedType.AddMember("condition", "IsEquippedType", allocator);
            rapidjson::Value typeVal(rapidjson::kObjectType);
            typeVal.AddMember("value", config.category->equippedTypeValue, allocator);
            equippedType.AddMember("Type", typeVal, allocator);
            equippedType.AddMember("Left hand", false, allocator);  // Condição da mão direita (sempre presente)
            andConditions.PushBack(equippedType, allocator);
        }

        // MUDANÇA: Substituído o caso especial de adagas por uma verificação genérica "isDualWield".
        if (config.category->isDualWield) {
            rapidjson::Value equippedTypeL(rapidjson::kObjectType);
            equippedTypeL.AddMember("condition", "IsEquippedType", allocator);
            rapidjson::Value typeValL(rapidjson::kObjectType);
            typeValL.AddMember("value", config.category->equippedTypeValue, allocator);
            equippedTypeL.AddMember("Type", typeValL, allocator);
            equippedTypeL.AddMember("Left hand", true, allocator);  // Adiciona a condição da mão esquerda
            andConditions.PushBack(equippedTypeL, allocator);
        }

        AddCompareValuesCondition(andConditions, "cycle_instance", config.instance_index, allocator);

        // Apenas adiciona a condição de ordem se for um "Pai"
        if (config.order_in_playlist > 0) {
            AddCompareValuesCondition(andConditions, "testarone", config.order_in_playlist, allocator);
        }

        // --- NOVA LÓGICA DE CONDIÇÕES DIRECIONAIS E RANDOM ---

        if (config.pRandom) {
            // Usa a ordem da playlist (valor de "testarone") para a condição Random
            int randomValue = (config.order_in_playlist > 0) ? config.order_in_playlist : 1;
            AddRandomCondition(andConditions, randomValue, allocator);
        } else {
            // Se não for Random, verifica as direções.
            // A checkbox pDodge é ignorada e não gera condições.
            if (config.pFront)
                AddCompareValuesCondition(andConditions, "DirecionalCycleMoveset", 1, allocator);
            else if (config.pFrontRight)
                AddCompareValuesCondition(andConditions, "DirecionalCycleMoveset", 2, allocator);
            else if (config.pRight)
                AddCompareValuesCondition(andConditions, "DirecionalCycleMoveset", 3, allocator);
            else if (config.pBackRight)
                AddCompareValuesCondition(andConditions, "DirecionalCycleMoveset", 4, allocator);
            else if (config.pBack)
                AddCompareValuesCondition(andConditions, "DirecionalCycleMoveset", 5, allocator);
            else if (config.pBackLeft)
                AddCompareValuesCondition(andConditions, "DirecionalCycleMoveset", 6, allocator);
            else if (config.pLeft)
                AddCompareValuesCondition(andConditions, "DirecionalCycleMoveset", 7, allocator);
            else if (config.pFrontLeft)
                AddCompareValuesCondition(andConditions, "DirecionalCycleMoveset", 8, allocator);
        }

        categoryAndBlock.AddMember("Conditions", andConditions, allocator);
        innerConditions.PushBack(categoryAndBlock, allocator);
    }

    if (!innerConditions.Empty()) {
        masterOrBlock.AddMember("Conditions", innerConditions, allocator);
        conditions.PushBack(masterOrBlock, allocator);
    }

    FILE* fp;
    fopen_s(&fp, jsonPath.string().c_str(), "wb");
    if (!fp) {
        SKSE::log::error("Falha ao abrir o arquivo para escrita: {}", jsonPath.string());
        return;
    }
    char writeBuffer[65536];
    rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
    doc.Accept(writer);
    fclose(fp);
}

// ATUALIZADO: Apenas uma pequena modificação para garantir que o 'value' é tratado como double.
void AnimationManager::AddCompareValuesCondition(rapidjson::Value& conditionsArray, const std::string& graphVarName,
                                                 int value, rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value newCompare(rapidjson::kObjectType);
    newCompare.AddMember("condition", "CompareValues", allocator);
    newCompare.AddMember("requiredVersion", "1.0.0.0", allocator);
    rapidjson::Value valueA(rapidjson::kObjectType);
    valueA.AddMember("value", static_cast<double>(value), allocator);  // Garante que o valor é float/double no JSON
    newCompare.AddMember("Value A", valueA, allocator);
    newCompare.AddMember("Comparison", "==", allocator);
    rapidjson::Value valueB(rapidjson::kObjectType);
    valueB.AddMember("graphVariable", rapidjson::Value(graphVarName.c_str(), allocator), allocator);
    valueB.AddMember("graphVariableType", "Float", allocator);
    newCompare.AddMember("Value B", valueB, allocator);
    conditionsArray.PushBack(newCompare, allocator);
}

// NOVA FUNÇÃO HELPER: Adiciona uma condição "CompareValues" para um valor booleano.
// Usada para verificar as checkboxes de movimento (F, B, L, R, etc.).
void AnimationManager::AddCompareBoolCondition(rapidjson::Value& conditionsArray, const std::string& graphVarName,
                                               bool value, rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value newCompare(rapidjson::kObjectType);
    newCompare.AddMember("condition", "CompareValues", allocator);
    newCompare.AddMember("requiredVersion", "1.0.0.0", allocator);

    rapidjson::Value valueA(rapidjson::kObjectType);
    valueA.AddMember("value", value, allocator);
    newCompare.AddMember("Value A", valueA, allocator);

    newCompare.AddMember("Comparison", "==", allocator);

    rapidjson::Value valueB(rapidjson::kObjectType);
    valueB.AddMember("graphVariable", rapidjson::Value(graphVarName.c_str(), allocator), allocator);
    valueB.AddMember("graphVariableType", "bool", allocator);  // O tipo aqui é "bool"
    newCompare.AddMember("Value B", valueB, allocator);

    conditionsArray.PushBack(newCompare, allocator);
}

void AnimationManager::AddRandomCondition(rapidjson::Value& conditionsArray, int value,
                                          rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value newRandom(rapidjson::kObjectType);
    newRandom.AddMember("condition", "Random", allocator);
    newRandom.AddMember("requiredVersion", "2.3.0.0", allocator);

    rapidjson::Value state(rapidjson::kObjectType);
    state.AddMember("scope", "Local", allocator);
    state.AddMember("shouldResetOnLoopOrEcho", true, allocator);
    newRandom.AddMember("State", state, allocator);

    rapidjson::Value minVal(rapidjson::kObjectType);
    minVal.AddMember("value", static_cast<double>(value), allocator);
    newRandom.AddMember("Minimum random value", minVal, allocator);

    rapidjson::Value maxVal(rapidjson::kObjectType);
    maxVal.AddMember("value", static_cast<double>(value), allocator);
    newRandom.AddMember("Maximum random value", maxVal, allocator);

    newRandom.AddMember("Comparison", "==", allocator);

    rapidjson::Value numVal(rapidjson::kObjectType);
    numVal.AddMember("graphVariable", "CycleMovesetsRandom", allocator);
    numVal.AddMember("graphVariableType", "Float", allocator);
    newRandom.AddMember("Numeric value", numVal, allocator);

    conditionsArray.PushBack(newRandom, allocator);
}



// Toda a parte de user ta ca pra baixo

