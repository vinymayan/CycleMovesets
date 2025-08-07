#pragma once
#include <map>
#include <optional>

#include "Settings.h"  // Inclui as novas definições
#include "rapidjson/document.h"

struct FileSaveConfig;

class AnimationManager {
public:
    static AnimationManager& GetSingleton();
    void ScanAnimationMods();
    void DrawImGuiMenu();

private:
    std::map<std::string, WeaponCategory> _categories;
    std::vector<AnimationModDef> _allMods;
    bool _preserveConditions = false;
    bool _isAddModModalOpen = false;
    CategoryInstance* _instanceToAddTo = nullptr;
    ModInstance* _modInstanceToAddTo = nullptr;
    void ProcessTopLevelMod(const std::filesystem::path& modPath);
    void DrawAddModModal();
    void SaveAllSettings();
    void UpdateOrCreateJson(const std::filesystem::path& jsonPath, const std::vector<FileSaveConfig>& configs);
    void AddCompareValuesCondition(rapidjson::Value& conditionsArray, const std::string& graphVarName, int value,
                                   rapidjson::Document::AllocatorType& allocator);
};

struct FileSaveConfig {
    int instance_index;
    int order_in_playlist;
    const WeaponCategory* category;
};
