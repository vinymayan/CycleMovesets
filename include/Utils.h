#pragma once

#include "ClibUtil/singleton.hpp"  // Baseado no exemplo, para fácil acesso ao singleton
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "SkyPrompt/API.hpp"


RE::PlayerCharacter::GetSingleton()->SetGraphVariableFloat("Testarone",0.0f); 


namespace GlobalControl {
    // --- CONFIGURAÇÃO ---
    // Altere estes valores para corresponder à sua variável global no Creation Kit
    constexpr std::string_view ESP_NAME = "CycleMoveset.esp";
    constexpr std::string_view GLOBAL_EDITOR_ID = "NovoJeito";

    // Ponteiro para a variável global, será preenchido quando o jogo carregar
    inline RE::TESGlobal* g_targetGlobal = nullptr;

    // ID do nosso plugin com a API SkyPrompt
    inline SkyPromptAPI::ClientID g_clientID = 0;

    // --- DEFINIÇÃO DAS TECLAS E PROMPTS ---
    // Nota: Os números são DirectX Scan Codes. U=21, I=23, O=24.
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> key_U = {RE::INPUT_DEVICE::kKeyboard, 22};
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> key_I = {RE::INPUT_DEVICE::kKeyboard, 23};
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> key_O = {
        RE::INPUT_DEVICE::kKeyboard, 25};  // Assumi que "diminuir com U" foi um erro de digitação e usei a tecla O.

    // Criamos os prompts. Eles não precisam de texto, pois são apenas listeners.
    // ActionID 0: Incrementar
    const SkyPromptAPI::Prompt prompt_Increment("Next", 0, 0, SkyPromptAPI::PromptType::kSinglePress, 0,
                                                std::span(&key_U, 1));
    // ActionID 1: Resetar
    const SkyPromptAPI::Prompt prompt_Reset("Reset", 1, 0, SkyPromptAPI::PromptType::kSinglePress, 0, std::span(&key_I, 1));
    // ActionID 2: Decrementar
    const SkyPromptAPI::Prompt prompt_Decrement("Back", 2, 0, SkyPromptAPI::PromptType::kSinglePress, 0,
                                                std::span(&key_O, 1));

    // A classe Sink processa os eventos da API
    class Sink final : public SkyPromptAPI::PromptSink, public clib_util::singleton::ISingleton<Sink> {
    public:
        // Retorna a lista de prompts que queremos monitorar
        std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;

        // Função chamada quando um evento (ex: pressionar tecla) ocorre
        void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;

    private:
        // Um array para guardar todos os nossos prompts
        std::array<SkyPromptAPI::Prompt, 3> m_prompts = {prompt_Increment, prompt_Reset, prompt_Decrement};
    };

    // Função para iniciar a lógica do nosso plugin
    void Initialize();
}