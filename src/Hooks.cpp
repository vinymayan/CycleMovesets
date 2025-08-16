#include <algorithm>
#include <format>
#include <fstream>
#include <string>
#include "Events.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"



AnimationManager& AnimationManager::GetSingleton() {
    static AnimationManager instance;
    return instance;
}


// --- DESAFIO 1: Nova função para escanear a pasta do sub-moveset em busca de tags ---
void ScanSubAnimationFolderForTags(const std::filesystem::path& subAnimPath,
                                                     SubAnimationDef& subAnimDef) {
    if (!std::filesystem::exists(subAnimPath) || !std::filesystem::is_directory(subAnimPath)) {
        return;
    }

    subAnimDef.attackCount = 0;
    subAnimDef.powerAttackCount = 0;
    subAnimDef.hasIdle = false;

    for (const auto& fileEntry : std::filesystem::directory_iterator(subAnimPath)) {
        if (fileEntry.is_regular_file()) {
            std::string filename = fileEntry.path().filename().string();
            std::string lowerFilename = filename;
            std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

            if (filename.rfind("BFCO_Attack", 0) == 0) {  // Prefixo "BFCO_Attack"
                subAnimDef.attackCount++;
            }
            if (filename.rfind("BFCO_PowerAttack", 0) == 0) {  // Prefixo "BFCO_PowerAttack"
                subAnimDef.powerAttackCount++;
            }
            if (lowerFilename.find("idle") != std::string::npos) {  // Contém "idle"
                subAnimDef.hasIdle = true;
            }
        }
    }
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
                                                           {"Dual Maces", 4.0, true},
                                                           {"Unarmed", 0.0, true}};


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

    // Agora que temos todos os mods, vamos encontrar quais arquivos já gerenciamos.
    SKSE::log::info("Verificando arquivos previamente gerenciados...");
    _managedFiles.clear();
    for (const auto& mod : _allMods) {
        for (const auto& subAnim : mod.subAnimations) {
            if (std::filesystem::exists(subAnim.path)) {
                std::ifstream fileStream(subAnim.path);
                std::string content((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
                fileStream.close();
                if (content.find("OAR_CYCLE_MANAGER_CONDITIONS") != std::string::npos) {
                    _managedFiles.insert(subAnim.path);
                }
            }
        }
    }
    SKSE::log::info("Encontrados {} arquivos gerenciados.", _managedFiles.size());

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
    // -- -NOVA CHAMADA-- -
    // Agora que a biblioteca de mods (_allMods) está completa, carregamos a configuração da UI.
    LoadStanceConfigurations();
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
                // --- DESAFIO 1: Chamar a função de escaneamento de tags ---
                ScanSubAnimationFolderForTags(subEntry.path(), subAnimDef);
                modDef.subAnimations.push_back(subAnimDef);
            }
        }
        _allMods.push_back(modDef);
    }
}

// --- Lógica da Interface de Usuário ---
void AnimationManager::DrawAddModModal() {
    if (_isAddModModalOpen) {
        if (_instanceToAddTo) {
            ImGui::OpenPopup("Adicionar Moveset");
        } else if (_modInstanceToAddTo || _userMovesetToAddTo) {
            ImGui::OpenPopup("Adicionar Sub-Moveset");
        }
        _isAddModModalOpen = false;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 modal_list_size = ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f);
    ImVec2 center = ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    // Modal "Adicionar Moveset" (sem alterações, já estava correto)
    if (ImGui::BeginPopupModal("Adicionar Moveset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Biblioteca de Movesets");
        ImGui::Separator();
        ImGui::InputText("Filtrar", _movesetFilter, 128);
        if (ImGui::BeginChild("BibliotecaMovesets", ImVec2(modal_list_size), true)) {
            std::string filter_str = _movesetFilter;
            std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);
            for (size_t modIdx = 0; modIdx < _allMods.size(); ++modIdx) {
                const auto& modDef = _allMods[modIdx];
                std::string mod_name_str = modDef.name;
                std::transform(mod_name_str.begin(), mod_name_str.end(), mod_name_str.begin(), ::tolower);
                if (filter_str.empty() || mod_name_str.find(filter_str) != std::string::npos) {
                    
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
                    ImGui::SameLine(240);
                    ImGui::Text("%s", modDef.name.c_str());
                    
                }
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Fechar")) {
            strcpy_s(_movesetFilter, "");
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Modal "Adicionar Sub-Moveset" (COM AS CORREÇÕES)
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Adicionar Sub-Moveset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Biblioteca de Animações");
        ImGui::Separator();
        ImGui::InputText("Filtrar", _subMovesetFilter, 128);

        if (ImGui::BeginChild("BibliotecaSubMovesets", ImVec2(modal_list_size), true)) {
            std::string filter_str = _subMovesetFilter;
            std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

            for (size_t modIdx = 0; modIdx < _allMods.size(); ++modIdx) {
                const auto& modDef = _allMods[modIdx];
                std::string mod_name_str = modDef.name;
                std::transform(mod_name_str.begin(), mod_name_str.end(), mod_name_str.begin(), ::tolower);
                bool parent_matches = mod_name_str.find(filter_str) != std::string::npos;
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

                if (filter_str.empty() || parent_matches || child_matches) {
                    if (ImGui::TreeNode(modDef.name.c_str())) {
                        for (size_t subAnimIdx = 0; subAnimIdx < modDef.subAnimations.size(); ++subAnimIdx) {
                            const auto& subAnimDef = modDef.subAnimations[subAnimIdx];
                            std::string sub_name_str = subAnimDef.name;
                            std::transform(sub_name_str.begin(), sub_name_str.end(), sub_name_str.begin(), ::tolower);

                            if (filter_str.empty() || sub_name_str.find(filter_str) != std::string::npos) {
                                ImGui::PushID(static_cast<int>(
                                    modIdx * 1000 +
                                    subAnimIdx));  // <-- CORREÇÃO: PushID antes de qualquer item da linha.

                                

                                // <-- CORREÇÃO 1: Alinha o botão à direita com um espaçamento.
                                float button_width = 100.0f;

                                ImVec2 content_avail;
                                ImGui::GetContentRegionAvail(&content_avail);  // Pega a região disponível

                                if (ImGui::Button("Adicionar", ImVec2(button_width, 0))) {
                                    SubAnimationInstance newSubInstance;
                                    newSubInstance.sourceModIndex = modIdx;
                                    newSubInstance.sourceSubAnimIndex = subAnimIdx;
                                    const auto& sourceMod = _allMods[modIdx];
                                    const auto& sourceSubAnim = sourceMod.subAnimations[subAnimIdx];
                                    newSubInstance.sourceModName = sourceMod.name;
                                    newSubInstance.sourceSubName = sourceSubAnim.name;
                                    if (_modInstanceToAddTo) {
                                        _modInstanceToAddTo->subAnimationInstances.push_back(newSubInstance);
                                    } else if (_userMovesetToAddTo) {
                                        _userMovesetToAddTo->subAnimations.push_back(newSubInstance);
                                    }
                                }
                                // Se a largura disponível for maior que o botão, alinha
                                if (content_avail.x > button_width) {
                                    ImGui::SameLine(button_width +40);
                                } else {
                                    ImGui::SameLine();  // Fallback para evitar posições negativas
                                }

                                
                                ImGui::Text("%s", subAnimDef.name.c_str());
                                ImGui::PopID();  // <-- CORREÇÃO 2: PopID movido para DENTRO do loop, no final da
                                                 // iteração.
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Fechar")) {
            strcpy_s(_subMovesetFilter, "");  // Limpa o filtro ao fechar
            ImGui::CloseCurrentPopup();
        }
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
                        // int playlistEntryCounter = 1;  // Contador apenas para "Pais"
                        //  Loop para os Movesets (ModInstance)
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

                                // int lastParentNumber = 0;   // Armazena o número do último "Pai"
                                //  NOVO: Variável para marcar um sub-moveset para remoção
                                // int subInstanceToRemove = -1;

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

                                    // NOVO: Define as flags da tabela. SizingFixedFit permite colunas de tamanho fixo.
                                    ImGuiTableFlags flags =
                                        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV;
                                    if (ImGui::BeginTable("sub_moveset_layout", 2, flags)) {
                                        // NOVO: Define as colunas. A primeira estica, a segunda é fixa.
                                        // O tamanho da segunda coluna é calculado para caber todos os checkboxes.
                                        float checkbox_width =
                                            ImGui::GetFrameHeightWithSpacing() +1000;  // 11 checkboxes
                                        ImGui::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthStretch);
                                        ImGui::TableSetupColumn("Conditions", ImGuiTableColumnFlags_WidthFixed,
                                                                checkbox_width);

                                        // --- COLUNA 1: Informações Principais ---
                                        ImGui::TableNextColumn();

                                        ImGui::Checkbox("##subselect", &subInstance.isSelected);
                                        ImGui::SameLine();

                                        // NOVO: Agrupa o nome e as tags para que o Drag and Drop funcione em ambos.
                                        ImGui::BeginGroup();

                                        // Lógica para criar a label principal (igual ao código original)
                                        std::string label;
                                        if (modInstance.isSelected && subInstance.isSelected) {
                                            if (playlistNumbers.count(&subInstance)) {
                                                label = std::format("[{}] {}", playlistNumbers.at(&subInstance),
                                                                    originSubAnim.name);
                                            } else if (parentNumbersForChildren.count(&subInstance)) {
                                                int parentNum = parentNumbersForChildren.at(&subInstance);
                                                label = std::format(" -> [{}] {}", parentNum, originSubAnim.name);
                                            } else {
                                                label = originSubAnim.name;
                                            }
                                        } else {
                                            label = originSubAnim.name;
                                        }
                                        if (subInstance.sourceModIndex != modInstance.sourceModIndex) {
                                            label += std::format(" (by: {})", originMod.name);
                                        }

                                        // ALTERADO: Desenhamos o texto da label. Não usamos mais Selectable aqui.
                                        ImGui::Selectable(label.c_str(), false, 0,
                                                          ImVec2(0, ImGui::GetTextLineHeight()));

                                          // CORREÇÃO: O código de Drag and Drop AGORA funciona, pois está atrelado ao
                                        // Selectable acima.
                                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
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
        

                                        // ALTERADO: As tags agora são desenhadas abaixo da label, dentro do mesmo
                                        // grupo. Elas usam SameLine() entre si para ficarem na mesma linha.
                                        bool firstTag = true;
                                        if (originSubAnim.attackCount > 0) {
                                            if (!firstTag) ImGui::SameLine();
                                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[HitCombo: %d]",
                                                               originSubAnim.attackCount);
                                            firstTag = false;
                                        }
                                        if (originSubAnim.powerAttackCount > 0) {
                                            if (!firstTag) ImGui::SameLine();
                                            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[PA: %d]",
                                                               originSubAnim.powerAttackCount);
                                            firstTag = false;
                                        }
                                        if (originSubAnim.hasIdle) {
                                            if (!firstTag) ImGui::SameLine();
                                            ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "[Idle]");
                                            firstTag = false;
                                        }

                                        ImGui::EndGroup();  // Fim do grupo de Drag and Drop


                                        // --- COLUNA 2: Checkboxes de Condição ---
                                        ImGui::TableNextColumn();

                                        // MOVIDO: Todos os checkboxes agora estão na segunda coluna.
                                        // Eles usam SameLine() para se alinharem horizontalmente DENTRO da coluna.
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
                                        ImGui::Checkbox("Movement", &subInstance.pDodge);

                                        ImGui::EndTable();
                                    }
                                    // --- FIM DA NOVA ESTRUTURA COM TABELA ---


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
    SaveStanceConfigurations();
    SKSE::log::info("Gerando arquivos de condição para OAR...");
    std::map<std::filesystem::path, std::vector<FileSaveConfig>> fileUpdates;

    // 1. Loop através de cada CATEGORIA de arma
    for (auto& pair : _categories) {
        WeaponCategory& category = pair.second;

        // 2. Loop através de cada uma das 4 INSTÂNCIAS
        for (int i = 0; i < 4; ++i) {
            CategoryInstance& instance = category.instances[i];
            int playlistParentCounter = 1;  // Contador para os itens "Pai"
            int lastParentOrder = 0;        // Armazena o número do último "Pai"
            // 3. Loop através dos MOVESETS (ModInstance) na instância
            for (size_t mod_i = 0; mod_i < instance.modInstances.size(); ++mod_i) {
                ModInstance& modInstance = instance.modInstances[mod_i];

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

                        config.isParent = isParent;

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
    // Agora, verifique todos os arquivos que já gerenciamos.
    // Se algum deles não estiver na lista de atualizações ativas,
    // significa que ele foi removido e precisa ser desativado.
    for (const auto& managedPath : _managedFiles) {
        // Se o arquivo não está no mapa de atualizações, adicione-o com um vetor vazio.
        if (fileUpdates.find(managedPath) == fileUpdates.end()) {
            fileUpdates[managedPath] = {};  // Adiciona para a fila de desativação
        }
    }
    // Adiciona todos os novos arquivos à lista de gerenciados para o futuro.
    for (const auto& pair : fileUpdates) {
        _managedFiles.insert(pair.first);
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

    // ---> INÍCIO DA NOVA LÓGICA DE PRIORIDADE <---

    // 1. Lê a prioridade base do arquivo. Se não existir, usa um valor padrão (ex: 0).
    int basePriority = 200000000;
    if (doc.HasMember("priority") && doc["priority"].IsInt()) {
        basePriority;  //= doc["priority"].GetInt();
    }

    // 2. Determina se esta animação está sendo usada como "mãe" em QUALQUER uma das configurações.
    bool isUsedAsParent = false;
    for (const auto& config : configs) {
        if (config.isParent) {
            isUsedAsParent = true;
            break;  // Se encontrarmos um uso como "mãe", já podemos parar.
        }
    }

    // 3. Define a prioridade final. Se for usada como mãe, mantém a base.
    //    Se for usada APENAS como filha, incrementa a prioridade para garantir que ela sobrescreva a mãe.
    int finalPriority = isUsedAsParent ? basePriority : basePriority + 1;

    // 4. Aplica a prioridade final ao documento JSON.
    if (doc.HasMember("priority")) {
        doc["priority"].SetInt(finalPriority);
    } else {
        doc.AddMember("priority", finalPriority, allocator);
    }

    rapidjson::Value oldConditions(rapidjson::kArrayType);
    if (_preserveConditions && doc.HasMember("conditions") && doc["conditions"].IsArray()) {
        for (auto& cond : doc["conditions"].GetArray()) {
            if (cond.IsObject() && cond.HasMember("comment") && cond["comment"] == "OAR_CYCLE_MANAGER_CONDITIONS") {
                // Pula o nosso próprio bloco ao preservar, pois ele será reescrito
                continue;
            }
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

    // Passo 1: Mapear todas as direções usadas pelas "filhas" para cada "mãe" (playlist).
    // A chave do mapa é o 'order_in_playlist', o valor é um set com os números das direções.
    std::map<int, std::set<int>> childDirectionsByPlaylist;
    for (const auto& config : configs) {
        if (!config.isParent) {
            int playlistId = config.order_in_playlist;
            if (playlistId > 0) {  // Garante que é uma filha de uma playlist válida
                if (config.pFront) childDirectionsByPlaylist[playlistId].insert(1);
                if (config.pFrontRight) childDirectionsByPlaylist[playlistId].insert(2);
                if (config.pRight) childDirectionsByPlaylist[playlistId].insert(3);
                if (config.pBackRight) childDirectionsByPlaylist[playlistId].insert(4);
                if (config.pBack) childDirectionsByPlaylist[playlistId].insert(5);
                if (config.pBackLeft) childDirectionsByPlaylist[playlistId].insert(6);
                if (config.pLeft) childDirectionsByPlaylist[playlistId].insert(7);
                if (config.pFrontLeft) childDirectionsByPlaylist[playlistId].insert(8);
            }
        }
    }

    if (!configs.empty()) {
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
                if (config.isParent) {
                    // LÓGICA DA MÃE: Adicionar condições negadas para cada direção de filha.
                    const auto& childDirs = childDirectionsByPlaylist[config.order_in_playlist];
                    if (!childDirs.empty()) {
                        for (int dirValue : childDirs) {
                            AddNegatedCompareValuesCondition(andConditions, "DirecionalCycleMoveset", dirValue,
                                                             allocator);
                        }
                    }
                } else {
                    // ---> LÓGICA DE ATIVAÇÃO CORRIGIDA (RANDOM + DIRECIONAL) <---

                    // 1. Adiciona a condição Random se a checkbox estiver marcada.
                    //    Esta condição é adicionada diretamente ao bloco AND principal.
                    if (config.pRandom) {
                        AddRandomCondition(andConditions, config.order_in_playlist, allocator);
                    }

                    // 2. Coleta as condições direcionais, independentemente da condição Random.
                    rapidjson::Value directionalOrConditions(rapidjson::kArrayType);
                    if (config.pFront)
                        AddCompareValuesCondition(directionalOrConditions, "DirecionalCycleMoveset", 1, allocator);
                    if (config.pFrontRight)
                        AddCompareValuesCondition(directionalOrConditions, "DirecionalCycleMoveset", 2, allocator);
                    if (config.pRight)
                        AddCompareValuesCondition(directionalOrConditions, "DirecionalCycleMoveset", 3, allocator);
                    if (config.pBackRight)
                        AddCompareValuesCondition(directionalOrConditions, "DirecionalCycleMoveset", 4, allocator);
                    if (config.pBack)
                        AddCompareValuesCondition(directionalOrConditions, "DirecionalCycleMoveset", 5, allocator);
                    if (config.pBackLeft)
                        AddCompareValuesCondition(directionalOrConditions, "DirecionalCycleMoveset", 6, allocator);
                    if (config.pLeft)
                        AddCompareValuesCondition(directionalOrConditions, "DirecionalCycleMoveset", 7, allocator);
                    if (config.pFrontLeft)
                        AddCompareValuesCondition(directionalOrConditions, "DirecionalCycleMoveset", 8, allocator);

                    // 3. Se houver alguma condição direcional, cria o bloco OR e o adiciona
                    //    também ao bloco AND principal.
                    if (!directionalOrConditions.Empty()) {
                        rapidjson::Value orBlock(rapidjson::kObjectType);
                        orBlock.AddMember("condition", "OR", allocator);
                        orBlock.AddMember("Conditions", directionalOrConditions, allocator);
                        andConditions.PushBack(orBlock, allocator);
                    }
                }

                categoryAndBlock.AddMember("Conditions", andConditions, allocator);
                innerConditions.PushBack(categoryAndBlock, allocator);
            }
        }
        if (!innerConditions.Empty()) {
            masterOrBlock.AddMember("Conditions", innerConditions, allocator);
            conditions.PushBack(masterOrBlock, allocator);
        }
    }
    // Se a lista de configs ESTIVER VAZIA, geramos uma condição "kill switch".
    else {
        rapidjson::Value masterOrBlock(rapidjson::kObjectType);
        masterOrBlock.AddMember("condition", "OR", allocator);
        masterOrBlock.AddMember("comment", "OAR_CYCLE_MANAGER_CONDITIONS", allocator);
        rapidjson::Value innerConditions(rapidjson::kArrayType);

        rapidjson::Value andBlock(rapidjson::kObjectType);
        andBlock.AddMember("condition", "AND", allocator);
        rapidjson::Value andConditions(rapidjson::kArrayType);

        // Adiciona uma condição que sempre será falsa.
        // Assumindo que a variável "CycleMovesetDisable" nunca será 1.0 no seu behavior graph.
        AddCompareValuesCondition(andConditions, "CycleMovesetDisable", 1, allocator);

        andBlock.AddMember("Conditions", andConditions, allocator);
        innerConditions.PushBack(andBlock, allocator);
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

std::optional<size_t> AnimationManager::FindModIndexByName(const std::string& name) {
    for (size_t i = 0; i < _allMods.size(); ++i) {
        if (_allMods[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<size_t> AnimationManager::FindSubAnimIndexByName(size_t modIdx, const std::string& name) {
    if (modIdx >= _allMods.size()) return std::nullopt;
    const auto& modDef = _allMods[modIdx];
    for (size_t i = 0; i < modDef.subAnimations.size(); ++i) {
        if (modDef.subAnimations[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

// --- NOVA FUNÇÃO DE CARREGAMENTO ---
void AnimationManager::LoadStanceConfigurations() {
    SKSE::log::info("Iniciando carregamento das configurações de Stance...");
    const std::filesystem::path stancesRoot = "Data/SKSE/Plugins/CycleMovesets/Stances";

    if (!std::filesystem::exists(stancesRoot)) {
        SKSE::log::info("Diretório de Stances não encontrado. Nenhuma configuração carregada.");
        return;
    }

    // Limpa as instâncias atuais antes de carregar
    for (auto& pair : _categories) {
        for (auto& instance : pair.second.instances) {
            instance.modInstances.clear();
        }
    }

    for (auto& categoryPair : _categories) {
        WeaponCategory& category = categoryPair.second;
        std::filesystem::path categoryPath = stancesRoot / category.name;

        if (!std::filesystem::exists(categoryPath)) continue;

        for (int i = 0; i < 4; ++i) {
            std::filesystem::path instancePath = categoryPath / ("Instance" + std::to_string(i + 1) + "_Cycle.json");
            if (!std::filesystem::exists(instancePath)) continue;

            FILE* fp;
            fopen_s(&fp, instancePath.string().c_str(), "rb");
            if (!fp) continue;

            char readBuffer[65536];
            rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
            rapidjson::Document doc;
            doc.ParseStream(is);
            fclose(fp);

            if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("stances") || !doc["stances"].IsArray()) {
                SKSE::log::warn("Arquivo de stance mal formatado: {}", instancePath.string());
                continue;
            }

            CategoryInstance& targetInstance = category.instances[i];
            const auto& stances = doc["stances"].GetArray();

            for (const auto& stanceJson : stances) {
                if (!stanceJson.IsObject()) continue;

                ModInstance newModInstance;
                std::string movesetName = stanceJson["name"].GetString();

                auto modIdxOpt = FindModIndexByName(movesetName);
                if (!modIdxOpt) {
                    SKSE::log::warn("Moveset '{}' não encontrado na biblioteca ao carregar stance.", movesetName);
                    continue;
                }
                newModInstance.sourceModIndex = *modIdxOpt;

                if (stanceJson.HasMember("animations") && stanceJson["animations"].IsArray()) {
                    for (const auto& animJson : stanceJson["animations"].GetArray()) {
                        SubAnimationInstance newSubInstance;
                        newSubInstance.sourceModName = animJson["sourceModName"].GetString();
                        newSubInstance.sourceSubName = animJson["sourceSubName"].GetString();

                        // Preenche os índices para uso em tempo de execução
                        auto subModIdxOpt = FindModIndexByName(newSubInstance.sourceModName);
                        if (subModIdxOpt) {
                            newSubInstance.sourceModIndex = *subModIdxOpt;
                            auto subAnimIdxOpt = FindSubAnimIndexByName(*subModIdxOpt, newSubInstance.sourceSubName);
                            if (subAnimIdxOpt) {
                                newSubInstance.sourceSubAnimIndex = *subAnimIdxOpt;
                            } else {
                                SKSE::log::warn("Sub-animação '{}' não encontrada em '{}'",
                                                newSubInstance.sourceSubName, newSubInstance.sourceModName);
                                continue;
                            }
                        } else {
                            SKSE::log::warn("Mod de origem '{}' não encontrado para sub-animação.",
                                            newSubInstance.sourceModName);
                            continue;
                        }

                        // Carrega os estados dos checkboxes
                        if (animJson.HasMember("pFront")) newSubInstance.pFront = animJson["pFront"].GetBool();
                        if (animJson.HasMember("pBack")) newSubInstance.pBack = animJson["pBack"].GetBool();
                        if (animJson.HasMember("pLeft")) newSubInstance.pLeft = animJson["pLeft"].GetBool();
                        if (animJson.HasMember("pRight")) newSubInstance.pRight = animJson["pRight"].GetBool();
                        if (animJson.HasMember("pFrontRight"))
                            newSubInstance.pFrontRight = animJson["pFrontRight"].GetBool();
                        if (animJson.HasMember("pFrontLeft"))
                            newSubInstance.pFrontLeft = animJson["pFrontLeft"].GetBool();
                        if (animJson.HasMember("pBackRight"))
                            newSubInstance.pBackRight = animJson["pBackRight"].GetBool();
                        if (animJson.HasMember("pBackLeft")) newSubInstance.pBackLeft = animJson["pBackLeft"].GetBool();
                        if (animJson.HasMember("pRandom")) newSubInstance.pRandom = animJson["pRandom"].GetBool();
                        if (animJson.HasMember("pDodge")) newSubInstance.pDodge = animJson["pDodge"].GetBool();

                        newModInstance.subAnimationInstances.push_back(newSubInstance);
                    }
                }
                targetInstance.modInstances.push_back(newModInstance);
            }
        }
    }
    SKSE::log::info("Carregamento das configurações de Stance concluído.");
}

// --- NOVA FUNÇÃO DE SALVAMENTO ---
void AnimationManager::SaveStanceConfigurations() {
    SKSE::log::info("Iniciando salvamento das configurações de Stance...");
    const std::filesystem::path stancesRoot = "Data/SKSE/Plugins/CycleMovesets/Stances";

    for (const auto& categoryPair : _categories) {
        const WeaponCategory& category = categoryPair.second;
        std::filesystem::path categoryPath = stancesRoot / category.name;
        std::filesystem::create_directories(categoryPath);  // Garante que a pasta da categoria exista

        for (int i = 0; i < 4; ++i) {
            const CategoryInstance& instance = category.instances[i];
            std::filesystem::path instancePath = categoryPath / ("Instance" + std::to_string(i + 1) + "_Cycle.json");

            rapidjson::Document doc;
            doc.SetObject();
            auto& allocator = doc.GetAllocator();

            doc.AddMember("Category", rapidjson::Value(category.name.c_str(), allocator), allocator);

            rapidjson::Value stancesArray(rapidjson::kArrayType);

            for (const auto& modInst : instance.modInstances) {
                if (!modInst.isSelected) continue;  // Pula movesets desativados

                const auto& sourceMod = _allMods[modInst.sourceModIndex];

                rapidjson::Value stanceObj(rapidjson::kObjectType);
                rapidjson::Value typeValue(sourceMod.author == "Usuário" ? "user_moveset" : "moveset", allocator);
                stanceObj.AddMember("type", typeValue, allocator);
                stanceObj.AddMember("name", rapidjson::Value(sourceMod.name.c_str(), allocator), allocator);

                rapidjson::Value animationsArray(rapidjson::kArrayType);
                for (const auto& subInst : modInst.subAnimationInstances) {
                    if (!subInst.isSelected) continue;

                    const auto& animOriginMod = _allMods[subInst.sourceModIndex];
                    const auto& animOriginSub = animOriginMod.subAnimations[subInst.sourceSubAnimIndex];

                    rapidjson::Value animObj(rapidjson::kObjectType);
                    animObj.AddMember("sourceModName", rapidjson::Value(animOriginMod.name.c_str(), allocator),
                                      allocator);
                    animObj.AddMember("sourceSubName", rapidjson::Value(animOriginSub.name.c_str(), allocator),
                                      allocator);
                    animObj.AddMember("sourceConfigPath",
                                      rapidjson::Value(animOriginSub.path.string().c_str(), allocator), allocator);

                    // Salva todos os booleans
                    animObj.AddMember("pFront", subInst.pFront, allocator);
                    animObj.AddMember("pBack", subInst.pBack, allocator);
                    animObj.AddMember("pLeft", subInst.pLeft, allocator);
                    animObj.AddMember("pRight", subInst.pRight, allocator);
                    animObj.AddMember("pFrontRight", subInst.pFrontRight, allocator);
                    animObj.AddMember("pFrontLeft", subInst.pFrontLeft, allocator);
                    animObj.AddMember("pBackRight", subInst.pBackRight, allocator);
                    animObj.AddMember("pBackLeft", subInst.pBackLeft, allocator);
                    animObj.AddMember("pRandom", subInst.pRandom, allocator);
                    animObj.AddMember("pDodge", subInst.pDodge, allocator);

                    animationsArray.PushBack(animObj, allocator);
                }
                stanceObj.AddMember("animations", animationsArray, allocator);
                stancesArray.PushBack(stanceObj, allocator);
            }

            doc.AddMember("stances", stancesArray, allocator);

            // Escreve o arquivo JSON
            FILE* fp;
            fopen_s(&fp, instancePath.string().c_str(), "wb");
            if (fp) {
                char writeBuffer[65536];
                rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
                rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
                doc.Accept(writer);
                fclose(fp);
            }
        }
    }
    SKSE::log::info("Salvamento das configurações de Stance concluído.");
}

void AnimationManager::AddNegatedCompareValuesCondition(rapidjson::Value& conditionsArray,
                                                        const std::string& graphVarName, int value,
                                                        rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value newCompare(rapidjson::kObjectType);
    newCompare.AddMember("condition", "CompareValues", allocator);

    // ---> A ÚNICA DIFERENÇA ESTÁ AQUI <---
    newCompare.AddMember("negated", true, allocator);

    newCompare.AddMember("requiredVersion", "1.0.0.0", allocator);
    rapidjson::Value valueA(rapidjson::kObjectType);
    valueA.AddMember("value", static_cast<double>(value), allocator);
    newCompare.AddMember("Value A", valueA, allocator);
    newCompare.AddMember("Comparison", "==", allocator);
    rapidjson::Value valueB(rapidjson::kObjectType);
    valueB.AddMember("graphVariable", rapidjson::Value(graphVarName.c_str(), allocator), allocator);
    valueB.AddMember("graphVariableType", "Float", allocator);
    newCompare.AddMember("Value B", valueB, allocator);
    conditionsArray.PushBack(newCompare, allocator);
}
