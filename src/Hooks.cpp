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

                                    // A lógica de numeração Pai/Filho agora só se aplica a itens SELECIONADOS.
                                    if (!isChildDisabled) {
                                        // Lógica de formatação do label (Pai vs Filho)
                                        if (subInstance.pFront == false && subInstance.pBack == false &&
                                            subInstance.pLeft == false && subInstance.pRight == false &&
                                            subInstance.pFrontRight == false && subInstance.pFrontLeft == false &&
                                            subInstance.pBackRight == false && subInstance.pBackLeft == false &&
                                            subInstance.pRandom == false) {
                                            // É um "Pai" (checkbox desmarcada)
                                            lastParentNumber = playlistEntryCounter;  // Salva seu número
                                            label = std::format("[{}] {}", playlistEntryCounter, originSubAnim.name);
                                            playlistEntryCounter++;  // Incrementa para o próximo "Pai"
                                        } else {
                                            // É um "Filho" (checkbox marcada)
                                            if (lastParentNumber > 0) {
                                                // Se já existe um pai, usa o número dele com indentação
                                                label =
                                                    std::format(" -> [{}] {}", lastParentNumber, originSubAnim.name);
                                            } else {
                                                // Caso especial: é um filho sem pai acima dele
                                                label = std::format(" -> [?] {}", originSubAnim.name);
                                            }
                                        }
                                    } else {
                                        // Itens desmarcardos são exibidos sem número.
                                        label = originSubAnim.name;
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

                                    if (isChildDisabled) {
                                        ImGui::PopStyleColor();
                                    }

                                    ImGui::PopID();
                                }

                                ImGui::TreePop();

                            }  // 4. Removemos a cor no final, garantindo que o Push/Pop sempre aconteçam em par.
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
                        bool isParent =
                            !(config.pFront || config.pBack || config.pLeft || config.pRight || config.pFrontRight ||
                              config.pFrontLeft || config.pBackRight || config.pBackLeft || config.pRandom);

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
        if (config.order_in_playlist > 0) {
            AddCompareValuesCondition(andConditions, "testarone", config.order_in_playlist, allocator);
        }

        if (config.pFront) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_F", true, allocator);
        if (config.pBack) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_B", true, allocator);
        if (config.pLeft) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_L", true, allocator);
        if (config.pRight) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_R", true, allocator);
        if (config.pFrontRight) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_FR", true, allocator);
        if (config.pFrontLeft) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_FL", true, allocator);
        if (config.pBackRight) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_BR", true, allocator);
        if (config.pBackLeft) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_BL", true, allocator);
        if (config.pRandom) AddCompareBoolCondition(andConditions, "CycleMovesetMovement_Random", true, allocator);

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
// Novas funcoes para usermovesets

void AnimationManager::LoadUserMovesets() {
    _userMovesets.clear();
    const std::filesystem::path userMovesetsPath = "Data/SKSE/Plugins/CycleMovesets/UserMovesets.json";

    if (!std::filesystem::exists(userMovesetsPath)) {
        SKSE::log::info("Arquivo UserMovesets.json não encontrado. Nenhum moveset de usuário carregado.");
        return;
    }

    std::ifstream fileStream(userMovesetsPath);
    std::string jsonContent((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
    fileStream.close();

    rapidjson::Document doc;
    if (doc.Parse(jsonContent.c_str()).HasParseError() || !doc.IsArray()) {
        SKSE::log::error("Erro ao fazer parse do UserMovesets.json.");
        return;
    }

    for (const auto& userMovesetJson : doc.GetArray()) {
        if (!userMovesetJson.IsObject()) continue;

        UserMoveset loadedMoveset;
        if (userMovesetJson.HasMember("name") && userMovesetJson["name"].IsString()) {
            loadedMoveset.name = userMovesetJson["name"].GetString();
        }

        if (userMovesetJson.HasMember("submovesets") && userMovesetJson["submovesets"].IsArray()) {
            for (const auto& subAnimJson : userMovesetJson["submovesets"].GetArray()) {
                SubAnimationInstance subInstance;
                if (subAnimJson.HasMember("sourceModIndex") && subAnimJson["sourceModIndex"].IsUint()) {
                    subInstance.sourceModIndex = subAnimJson["sourceModIndex"].GetUint();
                }
                if (subAnimJson.HasMember("sourceSubAnimIndex") && subAnimJson["sourceSubAnimIndex"].IsUint()) {
                    subInstance.sourceSubAnimIndex = subAnimJson["sourceSubAnimIndex"].GetUint();
                }
                loadedMoveset.subAnimations.push_back(subInstance);
            }
        }
        _userMovesets.push_back(loadedMoveset);
    }
    SKSE::log::info("{} movesets de usuário carregados.", _userMovesets.size());
}

void AnimationManager::SaveUserMovesets() {
    const std::filesystem::path userMovesetsPath = "Data/SKSE/Plugins/CycleMovesets/UserMovesets.json";
    std::filesystem::create_directories(userMovesetsPath.parent_path());

    rapidjson::Document doc;
    doc.SetArray();
    auto& allocator = doc.GetAllocator();

    for (const auto& userMoveset : _userMovesets) {
        rapidjson::Value movesetObj(rapidjson::kObjectType);
        movesetObj.AddMember("name", rapidjson::Value(userMoveset.name.c_str(), allocator), allocator);

        rapidjson::Value subAnimsArray(rapidjson::kArrayType);
        for (const auto& subAnim : userMoveset.subAnimations) {
            rapidjson::Value subAnimObj(rapidjson::kObjectType);
            subAnimObj.AddMember("sourceModIndex", rapidjson::Value(subAnim.sourceModIndex), allocator);
            subAnimObj.AddMember("sourceSubAnimIndex", rapidjson::Value(subAnim.sourceSubAnimIndex), allocator);
            subAnimsArray.PushBack(subAnimObj, allocator);
        }
        movesetObj.AddMember("submovesets", subAnimsArray, allocator);
        doc.PushBack(movesetObj, allocator);
    }

    FILE* fp;
    fopen_s(&fp, userMovesetsPath.string().c_str(), "wb");
    if (!fp) {
        SKSE::log::error("Falha ao salvar UserMovesets.json.");
        return;
    }
    char writeBuffer[65536];
    rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
    doc.Accept(writer);
    fclose(fp);
    SKSE::log::info("Movesets de usuário salvos com sucesso.");
}

// Hooks.cpp (adicionar estas novas funções)

// Hooks.cpp (adicionar estas novas funções)

void AnimationManager::DrawUserMovesetEditor() {
    ImGui::Text("Editando o Moveset: %s", _workspaceMoveset.name.c_str());
    ImGui::Separator();

    if (ImGui::Button("Adicionar Sub-Moveset")) {
        // Prepara o modal para adicionar ao nosso "workspace"
        _isAddModModalOpen = true;
        _instanceToAddTo = nullptr;
        _modInstanceToAddTo = nullptr;
        _userMovesetToAddTo = &_workspaceMoveset;
    }
    ImGui::SameLine();
    if (ImGui::Button("Salvar Moveset")) {
        if (_editingMovesetIndex == -1) {  // Criando um novo
            _userMovesets.push_back(_workspaceMoveset);
        } else {  // Editando um existente
            _userMovesets[_editingMovesetIndex] = _workspaceMoveset;
        }
        SaveUserMovesets();
        RebuildUserMovesetLibrary();    // <-- refaz a lista
        _isEditingUserMoveset = false;  // Volta para a tela de listagem
        SKSE::log::info("Moveset '{}' salvo. Recarregue o menu para vê-lo na lista de 'Adicionar Moveset'.",
                        _workspaceMoveset.name);
        RE::DebugNotification("Moveset salvo! Recarregue o menu para usar.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancelar")) {
        _isEditingUserMoveset = false;  // Volta sem salvar
    }
    ImGui::Separator();

    // Lista de sub-movesets no workspace
    int subToRemove = -1;
    for (size_t i = 0; i < _workspaceMoveset.subAnimations.size(); ++i) {
        const auto& subInstance = _workspaceMoveset.subAnimations[i];
        const auto& sourceMod = _allMods[subInstance.sourceModIndex];
        const auto& sourceSubAnim = sourceMod.subAnimations[subInstance.sourceSubAnimIndex];

        ImGui::PushID(static_cast<int>(i));
        if (ImGui::Button("X")) {
            subToRemove = static_cast<int>(i);
        }
        ImGui::SameLine();
        ImGui::Text("%s (de: %s)", sourceSubAnim.name.c_str(), sourceMod.name.c_str());
        ImGui::PopID();
    }

    if (subToRemove != -1) {
        _workspaceMoveset.subAnimations.erase(_workspaceMoveset.subAnimations.begin() + subToRemove);
    }
}

void AnimationManager::DrawUserMovesetManager() {
    // Se estamos na tela de edição, desenha o editor e para por aqui.
    if (_isEditingUserMoveset) {
        DrawUserMovesetEditor();
        return;
    }

    // --- Tela principal de listagem ---
    if (ImGui::Button("Criar Novo Moveset")) {
        ImGui::OpenPopup("Nomear Novo Moveset");
        strcpy_s(_newMovesetNameBuffer, "");  // Limpa o buffer
    }

    // Popup para dar nome ao novo moveset
    if (ImGui::BeginPopupModal("Nomear Novo Moveset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Nome", _newMovesetNameBuffer, 128);
        if (ImGui::Button("Criar e Editar")) {
            if (strlen(_newMovesetNameBuffer) > 0) {
                _workspaceMoveset = UserMoveset{};  // Limpa o workspace
                _workspaceMoveset.name = _newMovesetNameBuffer;
                _editingMovesetIndex = -1;     // -1 significa que é um novo moveset
                _isEditingUserMoveset = true;  // Muda para a tela de edição
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancelar")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::Text("Meus Movesets Salvos:");

    // Lista de movesets já criados
    int movesetToRemove = -1;
    for (size_t i = 0; i < _userMovesets.size(); ++i) {
        const auto& userMoveset = _userMovesets[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::Text("%s", userMoveset.name.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Editar")) {
            _workspaceMoveset = userMoveset;  // Copia para o workspace
            _editingMovesetIndex = static_cast<int>(i);
            _isEditingUserMoveset = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Remover")) {
            movesetToRemove = static_cast<int>(i);
        }
        ImGui::PopID();
    }

    if (movesetToRemove != -1) {
        _userMovesets.erase(_userMovesets.begin() + movesetToRemove);
        SaveUserMovesets();  // Salva a remoção
    }
}

// Hooks.cpp (adicione esta nova função em qualquer lugar com as outras definições de função)

void AnimationManager::RebuildUserMovesetLibrary() {
    SKSE::log::info("Reconstruindo a biblioteca de movesets do usuário em tempo real...");

    // Remove todos os mods que foram previamente adicionados como "Usuário" para evitar duplicatas
    std::erase_if(_allMods, [](const AnimationModDef& mod) { return mod.author == "Usuário"; });

    // Readiciona os movesets da lista _userMovesets (que está atualizada em memória)
    for (const auto& userMoveset : _userMovesets) {
        AnimationModDef modDef;
        modDef.name = userMoveset.name;
        modDef.author = "Usuário";

        for (const auto& subInstance : userMoveset.subAnimations) {
            if (subInstance.sourceModIndex < _allMods.size()) {
                const auto& sourceMod = _allMods[subInstance.sourceModIndex];
                if (subInstance.sourceSubAnimIndex < sourceMod.subAnimations.size()) {
                    modDef.subAnimations.push_back(sourceMod.subAnimations[subInstance.sourceSubAnimIndex]);
                }
            }
        }
        _allMods.push_back(modDef);
    }
    SKSE::log::info("Biblioteca reconstruída. Total de {} mods.", _allMods.size());
}