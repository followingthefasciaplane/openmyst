#pragma once

#include "ModelScript.h"
#include <string>
#include <unordered_map>
#include <memory>

// SpriteManager - model script loading orchestrator.
//
// Binary: loadModelScripts at 0x0043c810
// Loads model script text files defining texture atlas animations.
// NPC scripts: scripts/npc/{model}.txt
// Player scripts: scripts/player/{male|female}/{part}.txt (with image prefix)

class SpriteManager
{
public:
    static SpriteManager& instance();

    // Initialize with asset base path (e.g., "../dreadmyst-scripts-db-assets")
    void init(const std::string& assetPath);

    // Load NPC model script by model name (e.g., "antlion_small").
    // Looks for scripts/npc/antlion_small.txt
    std::shared_ptr<ModelScript> getNpcSprite(const std::string& modelName);

    // Load player sprite layer by part name (e.g., "default_chest")
    // and gender (0=male, 1=female).
    // Looks for scripts/player/male/default_chest.txt or female/
    std::shared_ptr<ModelScript> getPlayerSprite(const std::string& partName, int gender);

    // Clear all cached scripts
    void clear();

private:
    SpriteManager() = default;

    std::string m_assetPath;
    std::unordered_map<std::string, std::shared_ptr<ModelScript>> m_scripts;
};

#define sSpriteMgr SpriteManager::instance()
