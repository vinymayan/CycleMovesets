#include "RE/A/Actor.h"
#include "Serialization.h"

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