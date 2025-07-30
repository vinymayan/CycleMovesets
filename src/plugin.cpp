#include "logger.h"
#include "Utils.h"

namespace GlobalControl {
    // Implementação da função GetPrompts() da classe Sink
    std::span<const SkyPromptAPI::Prompt> Sink::GetPrompts() const { return m_prompts; }

    // Implementação da função ProcessEvent() da classe Sink
    void Sink::ProcessEvent(SkyPromptAPI::PromptEvent event) const {
        // Verificamos se a variável global foi carregada com sucesso
        if (!g_targetGlobal) {
            SKSE::log::warn("Tecla pressionada, mas a Variavel Global alvo nao foi encontrada!");
            return;
        }

        // Só nos importamos com eventos do tipo 'kAccepted', que para 'kSinglePress' significa que a tecla foi
        // pressionada.
        if (event.type != SkyPromptAPI::PromptEventType::kAccepted) {
            return;
        }

        // Usamos o ActionID que definimos nos prompts para decidir o que fazer
        switch (event.prompt.eventID) {
            case 0:  // Incrementar (Tecla U)
                g_targetGlobal->value += 1.0f;
                SKSE::log::info("Variavel Global incrementada para: {}", g_targetGlobal->value);
                RE::DebugNotification(std::format("Variavel: {:.0f}", g_targetGlobal->value).c_str());
                break;

            case 1:  // Resetar (Tecla I)
                g_targetGlobal->value = 0.0f;
                SKSE::log::info("Variavel Global resetada para 0.");
                RE::DebugNotification(std::format("Variavel: {:.0f}", g_targetGlobal->value).c_str());
                break;

            case 2:  // Decrementar (Tecla O)
                g_targetGlobal->value -= 1.0f;
                SKSE::log::info("Variavel Global decrementada para: {}", g_targetGlobal->value);
                RE::DebugNotification(std::format("Variavel: {:.0f}", g_targetGlobal->value).c_str());
                break;
        }

        // Reenviamos os prompts para a API para que ela continue a escutar por futuras pressões.
        // Algumas APIs consomem o prompt após o uso, então esta é uma boa prática.
        if (!SkyPromptAPI::SendPrompt(this, g_clientID)){
        }
    }

}

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Start
        // 1. Encontrar a variável global
        GlobalControl::g_targetGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>(GlobalControl::GLOBAL_EDITOR_ID);
        if (!GlobalControl::g_targetGlobal) {
            SKSE::log::critical("Nao foi possivel encontrar a Variavel Global com EditorID: {}",
                                GlobalControl::GLOBAL_EDITOR_ID);
            return;
        }
        SKSE::log::info("Variavel Global '{}' encontrada com sucesso.", GlobalControl::GLOBAL_EDITOR_ID);

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
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // Post-load
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    return true;
}
