#pragma once

#include "ClibUtil/singleton.hpp"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "SkyPrompt/API.hpp"
#include "Hooks.h"

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
    inline std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> Stance_key = {RE::INPUT_DEVICE::kKeyboard,
                                                                             Settings::hotkey_principal};
    inline std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> Moveset_key = {RE::INPUT_DEVICE::kKeyboard,
                                                                          Settings::hotkey_segunda};
    inline std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> Next_key = {RE::INPUT_DEVICE::kKeyboard,
                                                                          Settings::hotkey_terceira};
    inline std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> Reset_key = {RE::INPUT_DEVICE::kKeyboard,
                                                                           Settings::hotkey_quarta};
    inline std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID> Back_key = {RE::INPUT_DEVICE::kKeyboard,
                                                                            Settings::hotkey_quinta};

    // Criamos os prompts. Eles não precisam de texto, pois são apenas listeners.


    // ActionID 0: Incrementar
    inline SkyPromptAPI::Prompt prompt_Increment("Next", 3, 0, SkyPromptAPI::PromptType::kSinglePress, 20,
                                                std::span(&Next_key, 1));
    // ActionID 1: Resetar
    inline SkyPromptAPI::Prompt prompt_Reset("Reset", 3, 0, SkyPromptAPI::PromptType::kSinglePress, 20,
                                            std::span(&Reset_key, 1));
    // ActionID 2: Decrementar
    inline SkyPromptAPI::Prompt prompt_Decrement("Back", 2, 0, SkyPromptAPI::PromptType::kSinglePress, 20,
                                                 std::span(&Back_key, 1));
    inline SkyPromptAPI::Prompt menu_stance("Stance menu", 0, 0, SkyPromptAPI::PromptType::kHoldAndKeep, 20,
                                            std::span(&Stance_key, 1), 0, 600.0);
    inline SkyPromptAPI::Prompt menu_moveset("Moveset menu", 1, 0, SkyPromptAPI::PromptType::kHoldAndKeep, 20,
                                             std::span(&Moveset_key, 1),0,1.0);

    
    // A definimos como 'inline' aqui mesmo para simplificar e evitar problemas de linker.
    inline void UpdateRegisteredHotkeys() {
        SKSE::log::info("Atualizando hotkeys registradas na SkyPromptAPI...");

        // Atribui o novo valor do scan code (a segunda parte do 'pair')
        Stance_key.second = Settings::hotkey_principal;
        Moveset_key.second = Settings::hotkey_segunda;
        Next_key.second = Settings::hotkey_terceira;
        Reset_key.second = Settings::hotkey_quarta;
        Back_key.second = Settings::hotkey_quinta;

 
    }


    // A classe Sink processa os eventos da API
    class StancesSink final : public SkyPromptAPI::PromptSink, public clib_util::singleton::ISingleton<StancesSink> {
    public:
        // Retorna a lista de prompts que queremos monitorar
        std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;

        // Função chamada quando um evento (ex: pressionar tecla) ocorre
        void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        mutable bool except = false;
    private:
        // Um array para guardar todos os nossos prompts
        std::array<SkyPromptAPI::Prompt, 1> prompts = {menu_stance};
        
    };

        class StancesChangesSink final : public SkyPromptAPI::PromptSink,
                                     public clib_util::singleton::ISingleton<StancesChangesSink> {

    public:
        // Retorna a lista de prompts que queremos monitorar
        std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;

        // Função chamada quando um evento (ex: pressionar tecla) ocorre
        void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        mutable bool except = false;
    private:
        // Um array para guardar todos os nossos prompts
        std::array<SkyPromptAPI::Prompt, 2> prompts = {prompt_Increment, prompt_Decrement};
    };

            // A classe Sink processa os eventos da API
    class MovesetSink final : public SkyPromptAPI::PromptSink, public clib_util::singleton::ISingleton<MovesetSink> {
    public:
        // Retorna a lista de prompts que queremos monitorar
        std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;

        // Função chamada quando um evento (ex: pressionar tecla) ocorre
        void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        mutable bool except = false;
    private:
        // Um array para guardar todos os nossos prompts
        std::array<SkyPromptAPI::Prompt, 1> prompts = {menu_moveset};
    };

    class MovesetChangesSink final : public SkyPromptAPI::PromptSink,
                                     public clib_util::singleton::ISingleton<MovesetChangesSink> {
    public:
        // Retorna a lista de prompts que queremos monitorar
        std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;

        // Função chamada quando um evento (ex: pressionar tecla) ocorre
        void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;
        mutable bool except = false;
    private:
        // Um array para guardar todos os nossos prompts
        std::array<SkyPromptAPI::Prompt, 2> prompts = {prompt_Increment, prompt_Decrement};
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

    struct CameraEvent {
    public:
        RE::TESCameraState* oldState;
        RE::TESCameraState* newState;
    };

    class CameraChange : public RE::BSTEventSink<SKSE::CameraEvent> {
        
    public:
        static CameraChange* GetSingleton() {
            static CameraChange singleton;
            return &singleton;
        }
        RE::BSEventNotifyControl ProcessEvent(const SKSE::CameraEvent* a_event,
                                              RE::BSTEventSource<SKSE::CameraEvent>*) override;
    };

}