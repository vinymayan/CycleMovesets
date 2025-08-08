#include "Manager.h"  // Precisamos do nosso gerenciador de dados
#include "Events.h"
#include "Settings.h"

// ALTERAÇÃO AQUI: Incluímos o cabeçalho do framework
#include "SKSEMCP/SKSEMenuFramework.hpp"

void __stdcall UI::Render() {
    AnimationManager::GetSingleton().DrawMainMenu();  // Chamando a função com o nome correto
}

void UI::RegisterMenu() {
    if (!SKSEMenuFramework::IsInstalled()) {
        SKSE::log::warn("SKSE Menu Framework não encontrado.");
        return;
    }
    SKSE::log::info("SKSE Menu Framework encontrado. Registrando o menu.");
    SKSEMenuFramework::SetSection("OAR Cycle Manager");
    SKSEMenuFramework::AddSectionItem("Gerenciador de Ciclos", UI::Render);
}