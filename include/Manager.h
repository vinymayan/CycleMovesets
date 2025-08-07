#pragma once

namespace UI {
    // Fun��o para registrar nosso menu no SKSE Menu Framework.
    // Ser� chamada no carregamento do plugin.
    void RegisterMenu();

    // A fun��o que o SKSE Menu Framework chamar� para desenhar nosso menu.
    // O `__stdcall` � uma conven��o de chamada que a API do Windows (e o framework) espera.
    void __stdcall Render();
}