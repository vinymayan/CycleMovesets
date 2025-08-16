#include "logger.h"
#include "Utils.h"
#include "Events.h"
#include "Manager.h"
#include "Serialization.h"

namespace fs = std::filesystem;

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kInputLoaded) {
    }

    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        SKSE::GetCameraEventSource()->AddEventSink(GlobalControl::CameraChange::GetSingleton());
    }

    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // 2. Requisitar um ClientID da API SkyPrompt
        auto* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
        if (inputDeviceManager) {
            inputDeviceManager->AddEventSink(InputListener::GetSingleton());
            SKSE::log::info("Listener de input registrado com sucesso!");
        }

        if (auto* ui = RE::UI::GetSingleton(); ui) {
            logger::info("Adding event sink for dialogue menu auto zoom.");
            ui->AddEventSink<RE::MenuOpenCloseEvent>(GlobalControl::MenuOpen::GetSingleton());
        }
        GlobalControl::g_clientID = SkyPromptAPI::RequestClientID();
        if (GlobalControl::g_clientID > 0) {
            SKSE::log::info("ClientID {} recebido da SkyPromptAPI.", GlobalControl::g_clientID);
            SkyPromptAPI::RequestTheme(GlobalControl::g_clientID, "Cycle Movesets");
            // 3. Enviar nossos prompts para a API come�ar a monitorar as teclas
            if (SkyPromptAPI::SendPrompt(GlobalControl::StancesSink::GetSingleton(), GlobalControl::g_clientID)) {
                SKSE::log::info("Prompts de tecla registrados com sucesso!");
            } else {
                SKSE::log::error("Falha ao registrar os prompts de tecla na SkyPromptAPI.");
            }
            if (SkyPromptAPI::SendPrompt(GlobalControl::MovesetSink::GetSingleton(), GlobalControl::g_clientID)) {
                SKSE::log::info("Prompts de tecla registrados com sucesso!");
            } else {
                SKSE::log::error("Falha ao registrar os prompts de tecla na SkyPromptAPI.");
            }
        } else {
            SKSE::log::error("Falha ao obter um ClientID da SkyPromptAPI. A API esta instalada?");
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    
    // Registra seu ouvinte de eventos de A��o (sacar/guardar arma)
    auto* eventSource = SKSE::GetActionEventSource();
    if (eventSource) {
        eventSource->AddEventSink(GlobalControl::ActionEventHandler::GetSingleton());
        SKSE::log::info("Ouvinte de eventos de acao registrado com sucesso!");
    }

     // 1. Escaneia os arquivos de anima��o para carregar os dados.
    AnimationManager::GetSingleton().ScanAnimationMods();

    // 2. ALTERA��O AQUI: Chame a fun��o para registrar o menu no framework.
    UI::RegisterMenu();



    return true;
}
