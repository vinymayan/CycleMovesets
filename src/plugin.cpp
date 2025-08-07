#include "logger.h"
#include "Utils.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include <cstdio>  // Para FILE*
#include "Events.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "Settings.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "Manager.h"
#include "Hooks.h"

namespace fs = std::filesystem;


namespace GlobalControl {
    // Implementação da função GetPrompts() da classe Sink
    std::span<const SkyPromptAPI::Prompt> Sink::GetPrompts() const { return m_prompts; }

    // Implementação da função ProcessEvent() da classe Sink
    void Sink::ProcessEvent(SkyPromptAPI::PromptEvent event) const {
        // <-- MODIFICADO: Só processa a tecla se a arma estiver sacada -->
        if (!g_isWeaponDrawn) {
            return;
        }
        // Só nos importamos com eventos do tipo 'kAccepted', que para 'kSinglePress' significa que a tecla foi
        // pressionada.
        if (event.type != SkyPromptAPI::PromptEventType::kAccepted) {
            return;
        }
        static float cycleplayer = 0.0f;
        // Usamos o ActionID que definimos nos prompts para decidir o que fazer
        switch (event.prompt.eventID) {
            case 0:  // Incrementar (Tecla 1)
                SKSE::log::info("Apertou tecla 1");
                RE::DebugNotification(std::format("Apertou tecla 1").c_str());
                break;

            case 1:  // Incrementar (Tecla 2)
                SKSE::log::info("Apertou tecla 2");
                RE::DebugNotification(std::format("Apertou tecla 2").c_str());
                break;

            case 2:  // Incrementar (Tecla 3)
                SKSE::log::info("Apertou tecla 3");
                RE::DebugNotification(std::format("Apertou tecla 3").c_str());
                break;

            case 3:  // Incrementar (Tecla U)
                SKSE::log::info("Apertou tecla 4");
                RE::DebugNotification(std::format("Apertou tecla 4").c_str());
                break;

            case 4:  // Incrementar (Tecla U)
                cycleplayer += 1.0f;
                SKSE::log::info("Variavel Global incrementada para: {}", cycleplayer);
                RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("testarone", cycleplayer);
                RE::DebugNotification(std::format("Variavel: {:.0f}", cycleplayer).c_str());
                break;

            case 5:  // Resetar (Tecla I)
                cycleplayer = 0.0f;
                SKSE::log::info("Variavel Global resetada para 0.");
                RE::DebugNotification(std::format("Variavel: {:.0f}", cycleplayer).c_str());
                RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("testarone", cycleplayer);
                break;

            case 6:  // Decrementar (Tecla O)
                cycleplayer -= 1.0f;
                SKSE::log::info("Variavel Global decrementada para: {}", cycleplayer);
                RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("testarone", cycleplayer);
                RE::DebugNotification(std::format("Variavel: {:.0f}", cycleplayer).c_str());
                break;
        }

        // Reenviamos os prompts para a API para que ela continue a escutar por futuras pressões.
        // Algumas APIs consomem o prompt após o uso, então esta é uma boa prática.
        //if (!SkyPromptAPI::SendPrompt(this, g_clientID)){
        //  SKSE::log::warn("Falha ao reenviar prompt para a SkyPromptAPI.");
        //}
    }
    // <-- NOVO: Implementação da lógica do ouvinte de eventos de ação -->
    RE::BSEventNotifyControl ActionEventHandler::ProcessEvent(const SKSE::ActionEvent* a_event,
                                                              RE::BSTEventSource<SKSE::ActionEvent>*) {
        if (a_event && a_event->actor && a_event->actor->IsPlayerRef()) {
            // Jogador começou a sacar a arma
            if (a_event->type == SKSE::ActionEvent::Type::kBeginDraw) {
                SKSE::log::info("Arma sacada, mostrando o menu.");
                g_isWeaponDrawn = true;  // Define nosso controle como verdadeiro
                // Envia os prompts para a API, fazendo o menu aparecer
                SkyPromptAPI::SendPrompt(Sink::GetSingleton(), g_clientID);
            }
            // Jogador terminou de guardar a arma
            else if (a_event->type == SKSE::ActionEvent::Type::kEndSheathe) {
                SKSE::log::info("Arma guardada, escondendo o menu.");
                g_isWeaponDrawn = false;  // Define nosso controle como falso
                // Limpa os prompts da API, fazendo o menu desaparecer
                SkyPromptAPI::RemovePrompt(Sink::GetSingleton(), g_clientID);
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
}

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {

    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // 2. Requisitar um ClientID da API SkyPrompt
        GlobalControl::g_clientID = SkyPromptAPI::RequestClientID();
        if (GlobalControl::g_clientID > 0) {
            SKSE::log::info("ClientID {} recebido da SkyPromptAPI.", GlobalControl::g_clientID);
            // 3. Enviar nossos prompts para a API começar a monitorar as teclas
            if (SkyPromptAPI::SendPrompt(GlobalControl::Sink::GetSingleton(), GlobalControl::g_clientID)) {
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
    
    // Registra seu ouvinte de eventos de Ação (sacar/guardar arma)
    auto* eventSource = SKSE::GetActionEventSource();
    if (eventSource) {
        eventSource->AddEventSink(GlobalControl::ActionEventHandler::GetSingleton());
        SKSE::log::info("Ouvinte de eventos de acao registrado com sucesso!");
    }

        // 1. Escaneia os arquivos de animação para carregar os dados.
    // É importante fazer isso antes que o menu possa ser aberto.
    AnimationManager::GetSingleton().ScanAnimationMods();

    // 2. ALTERAÇÃO AQUI: Chame a função para registrar o menu no framework.
    UI::RegisterMenu();


    return true;
}
