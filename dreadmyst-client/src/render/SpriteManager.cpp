#include "SpriteManager.h"
#include "core/Log.h"

SpriteManager& SpriteManager::instance()
{
    static SpriteManager inst;
    return inst;
}

void SpriteManager::init(const std::string& assetPath)
{
    m_assetPath = assetPath;
    LOG_INFO("SpriteManager: asset path = '%s'", assetPath.c_str());
}

std::shared_ptr<ModelScript> SpriteManager::getNpcSprite(const std::string& modelName)
{
    // Check cache
    std::string key = "npc:" + modelName;
    auto it = m_scripts.find(key);
    if (it != m_scripts.end())
        return it->second;

    // Load from scripts/npc/{modelName}.txt
    std::string path = m_assetPath + "/scripts/npc/" + modelName + ".txt";

    auto script = std::make_shared<ModelScript>();
    if (!script->loadFromFile(path))
    {
        LOG_INFO("SpriteManager: NPC script '%s' not found", path.c_str());
        m_scripts[key] = nullptr;
        return nullptr;
    }

    LOG_INFO("SpriteManager: loaded NPC '%s' (%d anims)",
             modelName.c_str(), script->getAnimationCount());

    m_scripts[key] = script;
    return script;
}

std::shared_ptr<ModelScript> SpriteManager::getPlayerSprite(const std::string& partName, int gender)
{
    // Cache key includes gender
    std::string genderStr = (gender == 1) ? "female" : "male";
    std::string key = "player:" + genderStr + ":" + partName;
    auto it = m_scripts.find(key);
    if (it != m_scripts.end())
        return it->second;

    // Load from scripts/player/{male|female}/{partName}.txt
    // Image prefix is "player_{male|female}_" (prepended to image= value)
    std::string path = m_assetPath + "/scripts/player/" + genderStr + "/" + partName + ".txt";
    std::string imagePrefix = "player_" + genderStr + "_";

    auto script = std::make_shared<ModelScript>();
    if (!script->loadFromFile(path, imagePrefix))
    {
        LOG_INFO("SpriteManager: player script '%s' not found", path.c_str());
        m_scripts[key] = nullptr;
        return nullptr;
    }

    LOG_INFO("SpriteManager: loaded player '%s/%s' (%d anims)",
             genderStr.c_str(), partName.c_str(), script->getAnimationCount());

    m_scripts[key] = script;
    return script;
}

void SpriteManager::clear()
{
    m_scripts.clear();
}
