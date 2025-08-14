#include "Manager.h"  // Precisamos do nosso gerenciador de dados
#include "Events.h"
#include "Settings.h"
#include "Hooks.h"
#include "Utils.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <fstream> // Para ler e escrever arquivos
#include <filesystem> // Para criar diretórios (C++17+)
// ALTERAÇÃO AQUI: Incluímos o cabeçalho do framework
#include "SKSEMCP/SKSEMenuFramework.hpp"

constexpr const char* settings_path = "Data/SKSE/Plugins/CycleMoveset_Settings.json";

void __stdcall UI::Render() {

    AnimationManager::GetSingleton().DrawMainMenu();  // Chamando a função com o nome correto
}

namespace MyMenu {
    // 1. Defina a função de renderização para a sua página no menu
    void __stdcall RenderKeybindPage() {
        ImGui::Text("Configure as teclas de atalho do seu mod.");
        ImGui::Separator();
        ImGui::Spacing();

        // 2. Chame a nossa função Keybind para criar o widget
        // O ícone de teclado foi pego do Font Awesome, como no seu exemplo
        MyMenu::Keybind("Hotkey Principal", &Settings::hotkey_principal);
        MyMenu::Keybind("Segunda Hotkey", &Settings::hotkey_segunda);
        MyMenu::Keybind("Terceira Hotkey", &Settings::hotkey_terceira);
        MyMenu::Keybind("Quarta Hotkey", &Settings::hotkey_quarta);
        // Você pode adicionar quantos keybinds quiser!
        // static int another_hotkey = 0;
        // MyMenu::Keybind("Outra Hotkey", &another_hotkey);
    }
    void SaveSettings() {
        SKSE::log::info("Salvando configurações...");

        rapidjson::Document doc;
        doc.SetObject();  // O documento JSON será um objeto {}

        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        // Adiciona cada hotkey ao documento JSON
        doc.AddMember("hotkey_principal", Settings::hotkey_principal, allocator);
        doc.AddMember("hotkey_segunda", Settings::hotkey_segunda, allocator);
        doc.AddMember("hotkey_terceira", Settings::hotkey_terceira, allocator);
        doc.AddMember("hotkey_quarta", Settings::hotkey_quarta, allocator);

        // Converte o JSON para uma string formatada
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        // Garante que o diretório Data/SKSE/Plugins exista
        std::filesystem::path config_path(settings_path);
        std::filesystem::create_directories(config_path.parent_path());

        // Escreve a string no arquivo
        std::ofstream file(settings_path);
        if (file.is_open()) {
            file << buffer.GetString();
            file.close();
            SKSE::log::info("Configurações salvas em {}", settings_path);
        } else {
            SKSE::log::error("Falha ao abrir o arquivo para salvar as configurações: {}", settings_path);
        }
    }

    //  NOVA FUNÇÃO: Carrega as configurações de um arquivo JSON
    void LoadSettings() {
        SKSE::log::info("Carregando configurações...");

        std::ifstream file(settings_path);
        if (!file.is_open()) {
            SKSE::log::info("Arquivo de configurações não encontrado. Usando valores padrão.");
            return;  // Sai se o arquivo não existir (primeira execução)
        }

        // Lê o conteúdo do arquivo para uma string
        std::string json_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        rapidjson::Document doc;
        doc.Parse(json_data.c_str());

        if (doc.HasParseError() || !doc.IsObject()) {
            SKSE::log::error("Falha ao analisar o arquivo de configurações. Usando valores padrão.");
            return;
        }

        // Lê cada valor do JSON e atualiza as variáveis
        if (doc.HasMember("hotkey_principal") && doc["hotkey_principal"].IsInt()) {
            Settings::hotkey_principal = doc["hotkey_principal"].GetInt();
        }
        if (doc.HasMember("hotkey_segunda") && doc["hotkey_segunda"].IsInt()) {
            Settings::hotkey_segunda = doc["hotkey_segunda"].GetInt();
        }
        if (doc.HasMember("hotkey_terceira") && doc["hotkey_terceira"].IsInt()) {
            Settings::hotkey_terceira = doc["hotkey_terceira"].GetInt();
        }
        if (doc.HasMember("hotkey_quarta") && doc["hotkey_quarta"].IsInt()) {
            Settings::hotkey_quarta = doc["hotkey_quarta"].GetInt();
        }

        SKSE::log::info("Configurações carregadas com sucesso.");

        // IMPORTANTE: Após carregar, atualize as hotkeys na SkyPromptAPI
        GlobalControl::UpdateRegisteredHotkeys();
    }
    // O CORPO INTEIRO DA FUNÇÃO QUE VOCÊ RECORTOU DE hooks.h VEM PARA CÁ
    void Keybind(const char* label, int* dx_key_ptr) {
        static std::map<const char*, bool> is_waiting_map;
        bool& is_waiting_for_key = is_waiting_map[label];

        // --- LÓGICA DE EXIBIÇÃO ---
        const char* button_text = g_dx_to_name_map.count(*dx_key_ptr) ? g_dx_to_name_map.at(*dx_key_ptr) : "[None]";

        if (is_waiting_for_key) {
            button_text = "[ ... ]";
        }
        ImGui::AlignTextToFramePadding(); 
        ImGui::Text("%s", label);
        ImGui::SameLine();
        if (ImGui::Button(button_text, ImVec2(120, 60))) {
            is_waiting_for_key = true;
        }

        // --- LÓGICA DE CAPTURA E CONVERSÃO ---
        if (is_waiting_for_key) {
            for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i++) {
                ImGuiKey key = static_cast<ImGuiKey>(i);
                if (ImGui::IsKeyPressed(key)) {
                    if (key == ImGuiKey_Escape) {
                        *dx_key_ptr = 0;
                    } else {
                        if (g_imgui_to_dx_map.count(key)) {
                            *dx_key_ptr = g_imgui_to_dx_map.at(key);
                        } else {
                            *dx_key_ptr = 0;
                        }
                    }
                    is_waiting_for_key = false;
                    GlobalControl::UpdateRegisteredHotkeys();
                    MyMenu::SaveSettings();
                    break;
                }
            }
        }
    }

}

void UI::RegisterMenu() {
    if (!SKSEMenuFramework::IsInstalled()) {
        SKSE::log::warn("SKSE Menu Framework não encontrado.");
        return;
    }
    SKSE::log::info("SKSE Menu Framework encontrado. Registrando o menu.");
    SKSEMenuFramework::SetSection("Cycle Movesets");
    SKSEMenuFramework::AddSectionItem("Gerenciador de Ciclos", UI::Render);
    SKSEMenuFramework::AddSectionItem("Settings", MyMenu::RenderKeybindPage);
    MyMenu::LoadSettings();
}

