#include "RE/A/Actor.h"
#include "Serialization.h"
#include "Utils.h"

// A variável global que você quer alterar


// Scancodes das teclas WASD
constexpr uint32_t W_KEY = 0x11;
constexpr uint32_t A_KEY = 0x1E;
constexpr uint32_t S_KEY = 0x1F;
constexpr uint32_t D_KEY = 0x20;

// Esta função é chamada a cada frame de input
RE::BSEventNotifyControl InputListener::ProcessEvent(RE::InputEvent* const* a_event,
                                                     RE::BSTEventSource<RE::InputEvent*>* a_eventSource) {
    if (!a_event || !*a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    bool umaTeclaDeMovimentoMudou = false;

    for (auto* event = *a_event; event; event = event->next) {
        if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
            continue;
        }
        auto* button = event->AsButtonEvent();
        if (!button) {
            continue;
        }

        const uint32_t scanCode = button->GetIDCode();

        // Lógica rigorosa de máquina de estados para cada tecla
        if (scanCode == W_KEY) {
            // Só mude para 'pressionado' se a tecla ESTIVER 'down' E nosso estado atual for 'solto'.
            if (button->IsDown() && !w_pressed) {
                w_pressed = true;
                umaTeclaDeMovimentoMudou = true;
            }
            // Só mude para 'solto' se a tecla ESTIVER 'up' E nosso estado atual for 'pressionado'.
            else if (button->IsUp() && w_pressed) {
                w_pressed = false;
                umaTeclaDeMovimentoMudou = true;
            }
        } else if (scanCode == A_KEY) {
            if (button->IsDown() && !a_pressed) {
                a_pressed = true;
                umaTeclaDeMovimentoMudou = true;
            } else if (button->IsUp() && a_pressed) {
                a_pressed = false;
                umaTeclaDeMovimentoMudou = true;
            }
        } else if (scanCode == S_KEY) {
            if (button->IsDown() && !s_pressed) {
                s_pressed = true;
                umaTeclaDeMovimentoMudou = true;
            } else if (button->IsUp() && s_pressed) {
                s_pressed = false;
                umaTeclaDeMovimentoMudou = true;
            }
        } else if (scanCode == D_KEY) {
            if (button->IsDown() && !d_pressed) {
                d_pressed = true;
                umaTeclaDeMovimentoMudou = true;
            } else if (button->IsUp() && d_pressed) {
                d_pressed = false;
                umaTeclaDeMovimentoMudou = true;
            }
        }
    }

    // Apenas recalcule a direção se uma das nossas teclas de movimento REALMENTE mudou de estado.
    if (umaTeclaDeMovimentoMudou) {
        UpdateDirectionalState();
    }

    return RE::BSEventNotifyControl::kContinue;
}

// Esta função calcula o valor final da sua variável
void InputListener::UpdateDirectionalState() {
    static float DirecionalCycleMoveset = 0.0f;
    float VariavelAnterior = DirecionalCycleMoveset;


    // A ordem dos 'if' é importante para priorizar as diagonais
    if (w_pressed && a_pressed) {
        DirecionalCycleMoveset = 8.0f;  // Noroeste
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    } else if (w_pressed && d_pressed) {
        DirecionalCycleMoveset = 2.0f;  // Nordeste
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    } else if (s_pressed && a_pressed) {
        DirecionalCycleMoveset = 6.0f;  // Sudoeste
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    } else if (s_pressed && d_pressed) {
        DirecionalCycleMoveset = 4.0f;  // Sudeste
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    } else if (w_pressed) {
        DirecionalCycleMoveset = 1.0f;  // Norte (Frente)
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    } else if (a_pressed) {
        DirecionalCycleMoveset = 7.0f;  // Oeste (Esquerda)
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    } else if (s_pressed) {
        DirecionalCycleMoveset = 5.0f;  // Sul (Trás)
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    } else if (d_pressed) {
        DirecionalCycleMoveset = 3.0f;  // Leste (Direita)
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    } else {
        DirecionalCycleMoveset = 0.0f;  // Parado
        RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("DirecionalCycleMoveset", DirecionalCycleMoveset);
    }

    // Opcional: só imprime no log se o valor mudar, para não poluir o log.
    if (VariavelAnterior != DirecionalCycleMoveset) {
        SKSE::log::info("DirecionalCycleMoveset alterado para: {}", DirecionalCycleMoveset);
        // Aqui você enviaria o valor para sua animação, por exemplo:
        // RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("MinhaVariavelDirecional",
        // DirecionalCycleMoveset);
    }
}

std::span<const SkyPromptAPI::Prompt> GlobalControl::StancesSink::GetPrompts() const {
    return prompts; }

void GlobalControl::StancesSink::ProcessEvent(SkyPromptAPI::PromptEvent event) const {
    auto eventype = event.type;
    if (!g_isWeaponDrawn) {
        return;
    }

    switch (eventype) {
        case SkyPromptAPI::kAccepted:
                if(!except) {
                except = true;
                SkyPromptAPI::RemovePrompt(MovesetSink::GetSingleton(), GlobalControl::g_clientID);
                if (!SkyPromptAPI::SendPrompt(StancesChangesSink::GetSingleton(), GlobalControl::g_clientID)) {
                    logger::error("Skyprompt didnt worked Stances Changes Sink");
                }
                break;
            }
                
        case SkyPromptAPI::kUp:
            except = false;
            SkyPromptAPI::RemovePrompt(StancesChangesSink::GetSingleton(), GlobalControl::g_clientID);
            if (!SkyPromptAPI::SendPrompt(MovesetSink::GetSingleton(), GlobalControl::g_clientID)) {
                logger::error("Skyprompt didnt worked Moveset Sink");
            }
            break;        
    }

}

std::span<const SkyPromptAPI::Prompt> GlobalControl::StancesChangesSink::GetPrompts() const {

    return prompts; }

void GlobalControl::StancesChangesSink::ProcessEvent(SkyPromptAPI::PromptEvent event) const {}

std::span<const SkyPromptAPI::Prompt> GlobalControl::MovesetSink::GetPrompts() const {
    return prompts; }

void GlobalControl::MovesetSink::ProcessEvent(SkyPromptAPI::PromptEvent event) const {
    auto eventype = event.type;
    if (!g_isWeaponDrawn) {
        return;
    }
    switch (eventype) {

        case SkyPromptAPI::kAccepted:
            if (!except) {
                except = true;
                SkyPromptAPI::RemovePrompt(StancesSink::GetSingleton(), GlobalControl::g_clientID);
                if (!SkyPromptAPI::SendPrompt(MovesetChangesSink::GetSingleton(), GlobalControl::g_clientID)) {
                    logger::error("Skyprompt didnt worked Stances Changes Sink");
                }
                break;
            }

        case SkyPromptAPI::kDeclined:
            RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("testarone", 0);
            break;

        case SkyPromptAPI::kUp:
            except = false;
            SkyPromptAPI::RemovePrompt(MovesetChangesSink::GetSingleton(), GlobalControl::g_clientID);
            SkyPromptAPI::RemovePrompt(MovesetSink::GetSingleton(), GlobalControl::g_clientID);
            if (!SkyPromptAPI::SendPrompt(StancesSink::GetSingleton(), GlobalControl::g_clientID)) {
                logger::error("Skyprompt didnt worked Moveset Sink");
            }
            if (!SkyPromptAPI::SendPrompt(MovesetSink::GetSingleton(), GlobalControl::g_clientID)) {
                logger::error("Skyprompt didnt worked Moveset Sink");
            }
            break;
    }
}

std::span<const SkyPromptAPI::Prompt> GlobalControl::MovesetChangesSink::GetPrompts() const { 
    return prompts; }
    

void GlobalControl::MovesetChangesSink::ProcessEvent(SkyPromptAPI::PromptEvent event) const {
    static float cycleplayer = 0.0f;
    switch (event.prompt.eventID) {
        case 2:  // Moveset anterior
            cycleplayer -= 1.0f;
            logger::info("Variavel Global decrementada para: {}", cycleplayer);
            RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("testarone", cycleplayer);
            RE::DebugNotification(std::format("Variavel: {:.0f}", cycleplayer).c_str());
            break;

        case 3:  // Proximo moveset
            cycleplayer += 1.0f;
            logger::info("Variavel Global incrementada para: {}", cycleplayer);
            RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("testarone", cycleplayer);
            RE::DebugNotification(std::format("Variavel: {:.0f}", cycleplayer).c_str());
            break;

        case 5:  // Resetar (Tecla I)
            cycleplayer = 0.0f;
            logger::info("Variavel Global resetada para 0.");
            RE::DebugNotification(std::format("Variavel: {:.0f}", cycleplayer).c_str());
            RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("testarone", cycleplayer);
            break;

    }
}

RE::BSEventNotifyControl GlobalControl::CameraChange::ProcessEvent(const SKSE::CameraEvent* a_event,
                                                          RE::BSTEventSource<SKSE::CameraEvent>*) {
    // Verificação de segurança para garantir que o evento não é nulo.
    auto playerCamera = RE::PlayerCamera::GetSingleton();
    playerCamera->IsInThirdPerson();
    if (!a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Obtém a instância (singleton) da câmera do jogador.
    if (!playerCamera) {
        return RE::BSEventNotifyControl::kContinue;
    }

    // Verifica se a câmera está no estado de terceira pessoa.
    if (playerCamera->IsInThirdPerson()) {
        SkyPromptAPI::RemovePrompt(StancesSink::GetSingleton(), g_clientID);
        SkyPromptAPI::RemovePrompt(MovesetSink::GetSingleton(), g_clientID);
        logger:: info("Camera is in Third Person. (1)");

    } else {

        logger::info("Camera is in First Person. (0)");
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl GlobalControl::ActionEventHandler::ProcessEvent(const SKSE::ActionEvent* a_event,
                                                                         RE::BSTEventSource<SKSE::ActionEvent>*) {
    
    auto player_cam = RE::PlayerCamera::GetSingleton();
    player_cam->IsInThirdPerson();
    if (!player_cam->IsInThirdPerson()) {
        return RE::BSEventNotifyControl::kContinue;
    }
    if (a_event && a_event->actor && a_event->actor->IsPlayerRef()) {
        // Jogador comeou a sacar a arma
        if (a_event->type == SKSE::ActionEvent::Type::kBeginDraw) {
            SKSE::log::info("Arma sacada, mostrando o menu.");
            g_isWeaponDrawn = true;  // Define nosso controle como verdadeiro
            // Envia os prompts para a API, fazendo o menu aparecer
            SkyPromptAPI::SendPrompt(StancesSink::GetSingleton(), g_clientID);
            SkyPromptAPI::SendPrompt(MovesetSink::GetSingleton(), g_clientID);
        }
        // Jogador terminou de guardar a arma
        else if (a_event->type == SKSE::ActionEvent::Type::kEndSheathe) {
            SKSE::log::info("Arma guardada, escondendo o menu.");
            g_isWeaponDrawn = false;  // Define nosso controle como falso
            // Limpa os prompts da API, fazendo o menu desaparecer
            SkyPromptAPI::RemovePrompt(StancesSink::GetSingleton(), g_clientID);
            SkyPromptAPI::RemovePrompt(MovesetSink::GetSingleton(), g_clientID);
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

