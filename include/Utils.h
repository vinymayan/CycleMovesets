#pragma once

#include "ClibUtil/singleton.hpp"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "SkyPrompt/API.hpp"

namespace GlobalControl {
    // --- CONFIGURAÇÃO ---
    // Altere estes valores para corresponder à sua variável global no Creation Kit
    //constexpr std::string_view ESP_NAME = "CycleMoveset.esp";
    //constexpr std::string_view GLOBAL_EDITOR_ID = "NovoJeito";

    // Ponteiro para a variável global, será preenchido quando o jogo carregar
    //inline RE::TESGlobal* g_targetGlobal = nullptr;

    // ID do nosso plugin com a API SkyPrompt
    inline SkyPromptAPI::ClientID g_clientID = 0;
    inline bool g_isWeaponDrawn = false;

    // --- DEFINIÇÃO DAS TECLAS E PROMPTS ---
    // Nota: Os números são DirectX Scan Codes. U=21, I=23, O=24.
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> insta_1 = {RE::INPUT_DEVICE::kKeyboard, 2};
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> insta_2 = {RE::INPUT_DEVICE::kKeyboard, 3};
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> insta_3 = {RE::INPUT_DEVICE::kKeyboard, 4};
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> insta_4 = {RE::INPUT_DEVICE::kKeyboard, 5};
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> key_U = {RE::INPUT_DEVICE::kKeyboard, 22};
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> key_I = {RE::INPUT_DEVICE::kKeyboard, 23};
    const std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> key_P = {RE::INPUT_DEVICE::kKeyboard, 25}; 

    // Criamos os prompts. Eles não precisam de texto, pois são apenas listeners.
    const SkyPromptAPI::Prompt stance_1("Stance", 0, 0, SkyPromptAPI::PromptType::kSinglePress, 0,std::span(&insta_1, 1));
    const SkyPromptAPI::Prompt stance_2("Stance", 1, 0, SkyPromptAPI::PromptType::kSinglePress, 0,
                                                std::span(&insta_2, 1));
    const SkyPromptAPI::Prompt stance_3("Stance", 2, 0, SkyPromptAPI::PromptType::kSinglePress, 0,
                                                std::span(&insta_3, 1));
    const SkyPromptAPI::Prompt stance_4("Stance", 3, 0, SkyPromptAPI::PromptType::kSinglePress, 0,
                                                std::span(&insta_4, 1));
    // ActionID 0: Incrementar
    const SkyPromptAPI::Prompt prompt_Increment("Next", 4, 0, SkyPromptAPI::PromptType::kSinglePress, 0,std::span(&key_U, 1));
    // ActionID 1: Resetar
    const SkyPromptAPI::Prompt prompt_Reset("Reset", 5, 0, SkyPromptAPI::PromptType::kSinglePress, 0, std::span(&key_I, 1));
    // ActionID 2: Decrementar
    const SkyPromptAPI::Prompt prompt_Decrement("Back", 6, 0, SkyPromptAPI::PromptType::kSinglePress, 0,
                                                std::span(&key_P, 1));

    // A classe Sink processa os eventos da API
    class Sink final : public SkyPromptAPI::PromptSink, public clib_util::singleton::ISingleton<Sink> {
    public:
        // Retorna a lista de prompts que queremos monitorar
        std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;

        // Função chamada quando um evento (ex: pressionar tecla) ocorre
        void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;

    private:
        // Um array para guardar todos os nossos prompts
        std::array<SkyPromptAPI::Prompt, 7> m_prompts = {stance_1, stance_2, stance_3, stance_4, prompt_Increment, prompt_Reset, prompt_Decrement};
    };

    // <-- NOVO: Classe para ouvir eventos de sacar/guardar arma -->
    class ActionEventHandler : public RE::BSTEventSink<SKSE::ActionEvent> {
    public:
        // Singleton para garantir uma única instância
        static ActionEventHandler* GetSingleton() {
            static ActionEventHandler singleton;
            return &singleton;
        }
        // Função que processa os eventos de ação do jogo
        RE::BSEventNotifyControl ProcessEvent(const SKSE::ActionEvent* a_event,
                                              RE::BSTEventSource<SKSE::ActionEvent>*) override;
    };
}