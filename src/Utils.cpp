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

    // Primeiro, atualizamos o estado de todas as teclas pressionadas neste frame
    for (auto* event = *a_event; event; event = event->next) {
        if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
            continue;
        }
        auto* button = event->AsButtonEvent();
        if (!button) {
            continue;
        }

        const uint32_t scanCode = button->GetIDCode();
        const bool isDown = button->IsDown();
        const bool isUp = button->IsUp();

        if (scanCode == W_KEY) {
            if (isDown)
                w_pressed = true;
            else if (isUp)
                w_pressed = false;
        } else if (scanCode == A_KEY) {
            if (isDown)
                a_pressed = true;
            else if (isUp)
                a_pressed = false;
        } else if (scanCode == S_KEY) {
            if (isDown)
                s_pressed = true;
            else if (isUp)
                s_pressed = false;
        } else if (scanCode == D_KEY) {
            if (isDown)
                d_pressed = true;
            else if (isUp)
                d_pressed = false;
        }
    }

    // Após processar todos os eventos do frame, calculamos o estado direcional final
    UpdateDirectionalState();

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