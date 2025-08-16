#pragma once

#include <Windows.h>

#include <map>
#include <string>

#include "SKSEMCP/SKSEMenuFramework.hpp"

// Namespace para organizar as configurações do seu mod
namespace Settings {
    // Variável para armazenar nossa hotkey.
    inline int my_hotkey = 0;  // Usar 'inline' em C++17+ evita problemas de definição múltipla.
    inline int hotkey_principal ;
    inline int hotkey_segunda;
    inline int hotkey_terceira = 4;
    inline int hotkey_quarta = 5;
    inline int hotkey_quinta = 6;
}

// Namespace para a nossa UI
namespace MyMenu {

    // --- A PONTE ENTRE OS SISTEMAS ---

    // MAPA 1: Converte o ImGuiKey (moderno) para o Scan Code do DirectX (o que você precisa salvar)
    const std::map<ImGuiKey, int> g_imgui_to_dx_map = {
        {ImGuiKey_1, 2},           {ImGuiKey_2, 3},           {ImGuiKey_3, 4},
        {ImGuiKey_4, 5},           {ImGuiKey_5, 6},           {ImGuiKey_6, 7},
        {ImGuiKey_7, 8},           {ImGuiKey_8, 9},           {ImGuiKey_9, 10},
        {ImGuiKey_0, 11},          {ImGuiKey_A, 30},          {ImGuiKey_B, 48},
        {ImGuiKey_C, 46},          {ImGuiKey_D, 32},          {ImGuiKey_E, 18},
        {ImGuiKey_F, 33},          {ImGuiKey_G, 34},          {ImGuiKey_H, 35},
        {ImGuiKey_I, 23},          {ImGuiKey_J, 36},          {ImGuiKey_K, 37},
        {ImGuiKey_L, 38},          {ImGuiKey_M, 50},          {ImGuiKey_N, 49},
        {ImGuiKey_O, 24},          {ImGuiKey_P, 25},          {ImGuiKey_Q, 16},
        {ImGuiKey_R, 19},          {ImGuiKey_S, 31},          {ImGuiKey_T, 20},
        {ImGuiKey_U, 22},          {ImGuiKey_V, 47},          {ImGuiKey_W, 17},
        {ImGuiKey_X, 45},          {ImGuiKey_Y, 21},          {ImGuiKey_Z, 44},
        {ImGuiKey_F1, 59},         {ImGuiKey_F2, 60},         {ImGuiKey_F3, 61},
        {ImGuiKey_F4, 62},         {ImGuiKey_F5, 63},         {ImGuiKey_F6, 64},
        {ImGuiKey_F7, 65},         {ImGuiKey_F8, 66},         {ImGuiKey_F9, 67},
        {ImGuiKey_F10, 68},        {ImGuiKey_F11, 87},        {ImGuiKey_F12, 88},
        {ImGuiKey_Space, 57},      {ImGuiKey_Enter, 28},      {ImGuiKey_KeypadEnter, 156},
        {ImGuiKey_Backspace, 14},  {ImGuiKey_Tab, 15},        {ImGuiKey_LeftCtrl, 29},
        {ImGuiKey_RightCtrl, 157}, {ImGuiKey_LeftShift, 42},  {ImGuiKey_RightShift, 54},
        {ImGuiKey_LeftAlt, 56},    {ImGuiKey_RightAlt, 184},  {ImGuiKey_Delete, 211},
        {ImGuiKey_Insert, 210},    {ImGuiKey_Home, 199},      {ImGuiKey_End, 207},
        {ImGuiKey_PageUp, 201},    {ImGuiKey_PageDown, 209},  {ImGuiKey_UpArrow, 200},
        {ImGuiKey_DownArrow, 208}, {ImGuiKey_LeftArrow, 203}, {ImGuiKey_RightArrow, 205},
        {ImGuiKey_Semicolon, 39},  {ImGuiKey_Equal, 13},     {ImGuiKey_Comma, 51},
        {ImGuiKey_Minus, 12},      {ImGuiKey_Period, 52},     {ImGuiKey_Slash, 53},
        {ImGuiKey_Backslash, 43}};

    // MAPA 2: Converte o Scan Code do DirectX para um Nome (o que você precisa exibir) - O seu mapa original.
    const std::map<int, const char*> g_dx_to_name_map = {
        {0, "[Nenhuma]"},
        {1, "Escape"},
        {2, "1"},
        {3, "2"},
        {4, "3"},
        {5, "4"},
        {6, "5"},
        {7, "6"},
        {8, "7"},
        {9, "8"},
        {10, "9"},
        {11, "0"},
        {12, "-"},
        {13, "="},
        {14, "Backspace"},
        {15, "Tab"},
        {16, "Q"},
        {17, "W"},
        {18, "E"},
        {19, "R"},
        {20, "T"},
        {21, "Y"},
        {22, "U"},
        {23, "I"},
        {24, "O"},
        {25, "P"},
        {28, "Enter"},
        {29, "Left Ctrl"},
        {30, "A"},
        {31, "S"},
        {32, "D"},
        {33, "F"},
        {34, "G"},
        {35, "H"},
        {36, "J"},
        {37, "K"},
        {38, "L"},
        {39, ";"},
        {42, "Left Shift"},
        {43, "\\"},
        {44, "Z"},
        {45, "X"},
        {46, "C"},
        {47, "V"},
        {48, "B"},
        {49, "N"},
        {50, "M"},
        {51, ","},
        {52, "."},
        {53, "/"},
        {54, "Right Shift"},
        {56, "Left Alt"},
        {57, "Spacebar"},
        {59, "F1"},
        {60, "F2"},
        {61, "F3"},
        {62, "F4"},
        {63, "F5"},
        {64, "F6"},
        {65, "F7"},
        {66, "F8"},
        {67, "F9"},
        {68, "F10"},
        {87, "F11"},
        {88, "F12"},
        {156, "Keypad Enter"},
        {157, "Right Ctrl"},
        {184, "Right Alt"},
        {199, "Home"},
        {200, "Up Arrow"},
        {201, "PgUp"},
        {203, "Left Arrow"},
        {205, "Right Arrow"},
        {207, "End"},
        {208, "Down Arrow"},
        {209, "PgDown"},
        {210, "Insert"},
        {211, "Delete"}
        // Adicionei a key 0 para o caso "Nenhuma" para simplificar.
    };

    /**
     * @brief Cria um botão de keybind interativo, usando a captura do ImGui e salvando como DirectX Scan Code.
     * @param label O ID único e texto a ser exibido ao lado do botão.
     * @param dx_key_ptr Um ponteiro para o inteiro onde o scan code da tecla será armazenado.
     */
    void Keybind(const char* label, int* dx_key_ptr);
    
}
