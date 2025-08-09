#include "Events.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include <format>
#include <fstream>
#include <string>
#include <algorithm>


// Toda parte dos movesets criados pelo user esta ca
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

void AnimationManager::RebuildUserMovesetLibrary() {
    SKSE::log::info("Reconstruindo a biblioteca de movesets do usuário em tempo real...");

    // Remove todos os mods que foram previamente adicionados como "Usuário" para evitar duplicatas
    _allMods.erase(std::remove_if(_allMods.begin(), _allMods.end(),
                                  [](const AnimationModDef& mod) { return mod.author == "Usuário"; }),
                   _allMods.end());

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