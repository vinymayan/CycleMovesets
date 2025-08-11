#pragma once
#include <array>
#include <filesystem>
#include <string>
#include <vector>

// --- Defini��es da Biblioteca ---
struct SubAnimationDef {
    std::string name;
    std::filesystem::path path;
};
struct AnimationModDef {
    std::string name;
    std::string author;
    std::vector<SubAnimationDef> subAnimations;
};

// --- Estruturas de Configura��o do Usu�rio ---
struct SubAnimationInstance {
    // --- ALTERADO: Usamos nomes para salvar/carregar. Os �ndices ser�o preenchidos em tempo de execu��o. ---
    std::string sourceModName;  // Nome do mod de origem (e.g., "BFCO")
    std::string sourceSubName;  // Nome da sub-anima��o de origem (e.g., "700036")
    size_t sourceModIndex;
    size_t sourceSubAnimIndex;
    bool isSelected = true;
    bool pFront = false;
    bool pBack = false;
    bool pLeft = false;
    bool pRight = false;
    bool pFrontRight = false;
    bool pFrontLeft = false;
    bool pBackRight = false;
    bool pBackLeft = false;
    bool pRandom = false;
    bool pDodge = false;
};

struct ModInstance {
    size_t sourceModIndex;
    bool isSelected = true;
    std::vector<SubAnimationInstance> subAnimationInstances;
};

struct CategoryInstance {
    std::vector<ModInstance> modInstances;
};

struct WeaponCategory {
    std::string name;
    double equippedTypeValue;
    int activeInstanceIndex = 0;
    bool isDualWield = false;
    std::array<CategoryInstance, 4> instances;
};

struct UserMoveset {
    std::string name;
    std::vector<SubAnimationInstance> subAnimations;
};