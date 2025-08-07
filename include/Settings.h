#pragma once
#include <array>
#include <filesystem>
#include <string>
#include <vector>

// --- Definições da Biblioteca ---
struct SubAnimationDef {
    std::string name;
    std::filesystem::path path;
};
struct AnimationModDef {
    std::string name;
    std::string author;
    std::vector<SubAnimationDef> subAnimations;
};

// --- Estruturas de Configuração do Usuário ---
struct SubAnimationInstance {
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
    std::array<CategoryInstance, 4> instances;
};
