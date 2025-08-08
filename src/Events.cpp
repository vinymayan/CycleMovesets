#include "Manager.h"  // Precisamos do nosso gerenciador de dados
#include "Events.h"
#include "Settings.h"

// ALTERA��O AQUI: Inclu�mos o cabe�alho do framework
#include "SKSEMCP/SKSEMenuFramework.hpp"

void __stdcall UI::Render() {
    AnimationManager::GetSingleton().DrawMainMenu();  // Chamando a fun��o com o nome correto
}

void UI::RegisterMenu() {
    if (!SKSEMenuFramework::IsInstalled()) {
        SKSE::log::warn("SKSE Menu Framework n�o encontrado.");
        return;
    }
    SKSE::log::info("SKSE Menu Framework encontrado. Registrando o menu.");
    SKSEMenuFramework::SetSection("OAR Cycle Manager");
    SKSEMenuFramework::AddSectionItem("Gerenciador de Ciclos", UI::Render);
}