#include "ContentDB.h"
#include "core/Log.h"

ContentDB& ContentDB::instance()
{
    static ContentDB inst;
    return inst;
}

ContentDB::~ContentDB()
{
    close();
}

bool ContentDB::open(const std::string& dbPath)
{
    if (m_db)
        close();

    int rc = sqlite3_open_v2(dbPath.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK)
    {
        LOG_ERROR("ContentDB: failed to open '%s': %s", dbPath.c_str(), sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    LOG_INFO("ContentDB: opened '%s'", dbPath.c_str());

    preloadNpcModels();
    preloadAnimationInfo();

    return true;
}

void ContentDB::close()
{
    if (m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
        m_npcModelsById.clear();
        m_npcModelsByName.clear();
        m_animDurations.clear();
    }
}

bool ContentDB::getMapInfo(int mapId, MapInfo& out)
{
    if (!m_db)
        return false;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, music, ambience, los_vision, start_x, start_y, start_o "
                      "FROM map WHERE id = ?;";

    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        LOG_ERROR("ContentDB: prepare failed: %s", sqlite3_errmsg(m_db));
        return false;
    }

    sqlite3_bind_int(stmt, 1, mapId);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out.id        = sqlite3_column_int(stmt, 0);
        out.name      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* music = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        out.music     = music ? music : "";
        const char* amb = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        out.ambience  = amb ? amb : "";
        out.losVision = sqlite3_column_int(stmt, 4);
        out.startX    = sqlite3_column_int(stmt, 5);
        out.startY    = sqlite3_column_int(stmt, 6);
        out.startO    = sqlite3_column_int(stmt, 7);
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

// --- NPC Models ---

void ContentDB::preloadNpcModels()
{
    if (!m_db) return;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, name, height FROM npc_models;";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR("ContentDB: failed to prepare npc_models query: %s", sqlite3_errmsg(m_db));
        return;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        NpcModelInfo info;
        info.id     = sqlite3_column_int(stmt, 0);
        const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.name   = n ? n : "";
        info.height = sqlite3_column_int(stmt, 2);
        if (info.height <= 0) info.height = 50;

        m_npcModelsById[info.id] = info;
        m_npcModelsByName[info.name] = info;
        ++count;
    }

    sqlite3_finalize(stmt);
    LOG_INFO("ContentDB: preloaded %d npc_models", count);
}

bool ContentDB::getNpcModel(int modelId, NpcModelInfo& out) const
{
    auto it = m_npcModelsById.find(modelId);
    if (it == m_npcModelsById.end())
        return false;
    out = it->second;
    return true;
}

bool ContentDB::getNpcModelByName(const std::string& name, NpcModelInfo& out) const
{
    auto it = m_npcModelsByName.find(name);
    if (it == m_npcModelsByName.end())
        return false;
    out = it->second;
    return true;
}

// --- Animation Info ---

void ContentDB::preloadAnimationInfo()
{
    if (!m_db) return;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT script_name, animation_id, duration_in_seconds FROM model_info;";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOG_ERROR("ContentDB: failed to prepare model_info query: %s", sqlite3_errmsg(m_db));
        return;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char* script = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int animId = sqlite3_column_int(stmt, 1);
        float duration = static_cast<float>(sqlite3_column_double(stmt, 2));

        if (script)
        {
            std::string key = std::string(script) + ":" + std::to_string(animId);
            m_animDurations[key] = duration;
            ++count;
        }
    }

    sqlite3_finalize(stmt);
    LOG_INFO("ContentDB: preloaded %d animation entries", count);
}

bool ContentDB::getAnimationDuration(const std::string& scriptName, int animId, float& outDuration) const
{
    std::string key = scriptName + ":" + std::to_string(animId);
    auto it = m_animDurations.find(key);
    if (it == m_animDurations.end())
        return false;
    outDuration = it->second;
    return true;
}

// --- Sprite Hotspot ---

bool ContentDB::getSpriteHotspot(const std::string& filename, SpriteHotspot& out)
{
    if (!m_db) return false;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT x_offset, y_offset FROM sprite_hotspot WHERE filename = ?;";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out.xOffset = sqlite3_column_int(stmt, 0);
        out.yOffset = sqlite3_column_int(stmt, 1);
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

// --- NPC Template ---

bool ContentDB::getNpcTemplateModelId(int entry, int& outModelId, float& outScale)
{
    if (!m_db) return false;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT model_id, model_scale FROM npc_template WHERE entry = ?;";

    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, entry);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        outModelId = sqlite3_column_int(stmt, 0);
        // model_scale in DB is stored as integer percentage (100 = 1.0x)
        int scaleInt = sqlite3_column_int(stmt, 1);
        outScale = (scaleInt > 0) ? static_cast<float>(scaleInt) / 100.f : 1.0f;
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}
