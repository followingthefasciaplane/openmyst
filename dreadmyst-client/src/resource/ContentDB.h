#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include <sqlite3.h>

// ContentDB - SQLite game.db reader.
//
// Binary: ContentMgr::db at 0x0043eed0 provides keyed lookups.
// The game.db has 55 tables.

struct MapInfo
{
    int id        = 0;
    std::string name;
    std::string music;
    std::string ambience;
    int losVision = 1;
    int startX    = 0;
    int startY    = 0;
    int startO    = 0;
};

struct NpcModelInfo
{
    int id          = 0;
    std::string name;
    int height      = 50;  // frame height in pixels (square frames: width == height)
};

struct AnimationInfo
{
    float durationSeconds = 0.f;
};

struct SpriteHotspot
{
    int xOffset = 0;
    int yOffset = 0;
};

class ContentDB
{
public:
    static ContentDB& instance();

    bool open(const std::string& dbPath);
    void close();
    bool isOpen() const { return m_db != nullptr; }

    // Map
    bool getMapInfo(int mapId, MapInfo& out);

    // NPC models (preloaded on open)
    bool getNpcModel(int modelId, NpcModelInfo& out) const;
    bool getNpcModelByName(const std::string& name, NpcModelInfo& out) const;

    // Animation durations (preloaded on open)
    bool getAnimationDuration(const std::string& scriptName, int animId, float& outDuration) const;

    // Sprite hotspot (queried on demand)
    bool getSpriteHotspot(const std::string& filename, SpriteHotspot& out);

    // NPC template -> model_id + model_scale (queried on demand)
    bool getNpcTemplateModelId(int entry, int& outModelId, float& outScale);

private:
    ContentDB() = default;
    ~ContentDB();

    void preloadNpcModels();
    void preloadAnimationInfo();

    sqlite3* m_db = nullptr;

    // Preloaded caches
    std::unordered_map<int, NpcModelInfo>    m_npcModelsById;     // id -> model
    std::unordered_map<std::string, NpcModelInfo> m_npcModelsByName; // name -> model

    // key = "scriptName:animId"
    std::unordered_map<std::string, float>   m_animDurations;
};

#define sContentDB ContentDB::instance()
