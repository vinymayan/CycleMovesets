#pragma once
#include <map>
#include <optional>
#include <string>
#include "Settings.h"  // Inclui as novas defini��es
#include "rapidjson/document.h"

struct FileSaveConfig;

class AnimationManager {
public:
    static AnimationManager& GetSingleton();
    void ScanAnimationMods();
    void DrawMainMenu();

private:
    std::map<std::string, WeaponCategory> _categories;
    std::vector<AnimationModDef> _allMods;
    bool _preserveConditions = false;
    bool _isAddModModalOpen = false;
    CategoryInstance* _instanceToAddTo = nullptr;
    ModInstance* _modInstanceToAddTo = nullptr;
    // NOVO: Vari�veis para o modal de cria��o de moveset
    ModInstance* _modInstanceToSaveAsCustom = nullptr;
    char _newMovesetNameBuffer[128] = "";

    void ProcessTopLevelMod(const std::filesystem::path& modPath);
    void DrawAddModModal();
    void SaveAllSettings();
    void UpdateOrCreateJson(const std::filesystem::path& jsonPath, const std::vector<FileSaveConfig>& configs);
    void AddCompareValuesCondition(rapidjson::Value& conditionsArray, const std::string& graphVarName, int value,
                                   rapidjson::Document::AllocatorType& allocator);
    // NOVA FUN��O HELPER: Para adicionar condi��es booleanas (checkboxes)
    void AddCompareBoolCondition(rapidjson::Value& conditionsArray, const std::string& graphVarName, bool value,
                                 rapidjson::Document::AllocatorType& allocator);

    // Fun��o do random aqui
    void AddRandomCondition(rapidjson::Value& conditionsArray, int value,
                                              rapidjson::Document::AllocatorType& allocator);

    // --- NOVAS VARI�VEIS PARA GERENCIAR MOVESETS DO USU�RIO ---

    // Estrutura para manter um moveset de usu�rio em mem�ria
    struct UserMoveset {
        std::string name;
        std::vector<SubAnimationInstance> subAnimations;
    };

    // Vetor com todos os movesets criados pelo usu�rio
    std::vector<UserMoveset> _userMovesets;

    // Estado da UI de cria��o/edi��o
    bool _isEditingUserMoveset = false;  // true quando estamos na tela de edi��o
    int _editingMovesetIndex = -1;       // �ndice do moveset sendo editado, -1 para um novo
    UserMoveset _workspaceMoveset;       // "Mesa de trabalho" para criar/editar um moveset

    // Ponteiro para saber onde adicionar um sub-moveset vindo do modal
    UserMoveset* _userMovesetToAddTo = nullptr;



    // --- NOVAS FUN��ES PRIVADAS ---
    void DrawAnimationManager();  // Movido para private pois � chamado por DrawMainMenu
    void LoadUserMovesets();
    void SaveUserMovesets();
    void DrawUserMovesetManager();  // A UI principal para esta nova se��o
    void DrawUserMovesetEditor();   // A UI para a tela de edi��o
    void RebuildUserMovesetLibrary();  // <-- ADICIONE ESTA LINHA

    // Filtro de pesquisa
    char _movesetFilter[128] = "";
    char _subMovesetFilter[128] = "";
    // Da load na ordem dos movesets e submovesets
    void LoadStateForSubAnimation(size_t modIdx, size_t subAnimIdx);
};

struct FileSaveConfig {
    int instance_index;
    int order_in_playlist;
    const WeaponCategory* category;
    // Campos adicionados para carregar o estado das checkboxes
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
