#pragma once
#include "core/Types.h"
#include "core/Log.h"
#include "database/MySQLConnector.h"

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

// ---------------------------------------------------------------------------
// Data structures populated from the database
// ---------------------------------------------------------------------------

struct DbCharacterListEntry {
    std::string name;
    uint32      playerClass = 0;
    uint32      level       = 0;
    Guid        guid        = 0;
    uint32      portrait    = 0;
    uint32      gender      = 0;
};

struct DbItemData {
    uint32 slot          = 0;
    uint32 entry         = 0;
    uint32 affix         = 0;
    uint32 affixScore    = 0;
    uint32 gem1          = 0;
    uint32 gem2          = 0;
    uint32 gem3          = 0;
    uint32 gem4          = 0;
    uint32 enchantLevel  = 0;
    uint32 soulbound     = 0;
    uint32 count         = 0;
    uint32 durability    = 0;
};

struct DbEquipmentData {
    uint32 entry         = 0;
    uint32 affix         = 0;
    uint32 affixScore    = 0;
    uint32 gem1          = 0;
    uint32 gem2          = 0;
    uint32 gem3          = 0;
    uint32 gem4          = 0;
    uint32 enchantLevel  = 0;
    uint32 soulbound     = 0;
    uint32 durability    = 0;
};

struct DbSpellData {
    uint32 spellId = 0;
    uint32 level   = 0;
};

struct DbSpellCooldownData {
    uint32      spellId   = 0;
    std::string startDate;
    std::string endDate;
};

struct DbAuraEffectTick {
    uint32 baseAmount  = 0;
    uint32 tickAmount  = 0;
    uint32 ticksLeft   = 0;
};

struct DbAuraData {
    uint32           spellId     = 0;
    uint32           casterGuid  = 0;
    uint32           duration    = 0;
    uint32           maxDuration = 0;
    uint32           stacks      = 0;
    DbAuraEffectTick effects[MAX_AURA_EFFECTS];
};

struct DbQuestData {
    uint32 questId          = 0;
    uint32 rewarded         = 0;
    uint32 objectiveCount1  = 0;
    uint32 objectiveCount2  = 0;
    uint32 objectiveCount3  = 0;
    uint32 objectiveCount4  = 0;
};

struct DbStatInvestment {
    uint32 statType = 0;
    uint32 amount   = 0;
};

struct DbMailItem {
    uint32 entry         = 0;
    uint32 affix         = 0;
    uint32 affixScore    = 0;
    uint32 count         = 0;
    uint32 gem1          = 0;
    uint32 gem2          = 0;
    uint32 gem3          = 0;
    uint32 gem4          = 0;
    uint32 enchantLevel  = 0;
    uint32 soulbound     = 0;
    uint32 durability    = 0;
};

// Full player record loaded from the database
struct DbPlayerRecord {
    Guid        guid            = 0;
    uint32      account         = 0;
    std::string name;
    uint32      playerClass     = 0;
    uint32      gender          = 0;
    uint32      portrait        = 0;
    uint32      level           = 0;
    float       hpPct           = 1.0f;
    float       mpPct           = 1.0f;
    uint32      xp              = 0;
    uint32      money           = 0;
    float       positionX       = 0.0f;
    float       positionY       = 0.0f;
    float       orientation     = 0.0f;
    uint32      map             = 0;
    uint32      homeMap         = 0;
    float       homeX           = 0.0f;
    float       homeY           = 0.0f;
    std::string logoutTime;
    std::string lastDeathTime;
    uint32      pvpPoints       = 0;
    uint32      pkCount         = 0;
    uint32      pvpFlag         = 0;
    uint32      progression     = 0;
    uint32      numInvestedSpells = 0;
    uint32      combatRating    = 0;
    uint32      timePlayed      = 0;    // IFNULL(apt.timeSecs, 0) AS timePlayed
    bool        isGM            = false;
    bool        isAdmin         = false;

    // Sub-collections
    std::vector<DbItemData>            inventory;
    std::vector<DbEquipmentData>       equipment;
    std::vector<DbItemData>            bank;
    std::vector<DbSpellData>           spells;
    std::vector<DbSpellCooldownData>   spellCooldowns;
    std::vector<DbAuraData>            auras;
    std::vector<DbQuestData>           quests;
    std::vector<DbStatInvestment>      statInvestments;
    std::vector<Guid>                  waypoints;
    std::vector<DbMailItem>            mail;
};

// Auth token result
struct DbAuthToken {
    uint32      userId = 0;
    std::string token;
    bool        valid  = false;
};

// ---------------------------------------------------------------------------
// PlayerDb  --  all player persistence SQL against MySQL
// ---------------------------------------------------------------------------

class PlayerDb {
public:
    PlayerDb() = default;

    // Set the MySQL connector to use.  Must be called before any queries.
    void setConnector(MySQLConnector* connector);

    // ------------------------------------------------------------------
    // Character list / creation queries
    // ------------------------------------------------------------------

    // Retrieve the character list for an account.
    bool getCharacterList(uint32 accountId, std::vector<DbCharacterListEntry>& outList);

    // Check whether a character name is already taken.
    bool isNameTaken(const std::string& name, bool& outTaken);

    // Get the highest player GUID currently in the database.
    bool getMaxGuid(Guid& outGuid);

    // ------------------------------------------------------------------
    // Full player load  (PlayerDb::loadPlayer)
    // ------------------------------------------------------------------

    // Load a complete player record by GUID.
    bool loadPlayer(Guid guid, DbPlayerRecord& outRecord);

    // ------------------------------------------------------------------
    // Auth tokens
    // ------------------------------------------------------------------

    // Validate an auth token (60-second window).
    bool validateAuthToken(const std::string& token, DbAuthToken& outAuth);

    // Validate login credentials against accounts table.
    bool validateLogin(const std::string& username, const std::string& password, uint32& outAccountId);

    // ------------------------------------------------------------------
    // Sub-record loaders (used internally by loadPlayer)
    // ------------------------------------------------------------------

    bool loadInventory(Guid guid, std::vector<DbItemData>& outItems);
    bool loadEquipment(Guid guid, std::vector<DbEquipmentData>& outItems);
    bool loadBank(Guid guid, std::vector<DbItemData>& outItems);
    bool loadSpells(Guid guid, std::vector<DbSpellData>& outSpells);
    bool loadSpellCooldowns(Guid guid, std::vector<DbSpellCooldownData>& outCooldowns);
    bool loadAuras(Guid guid, std::vector<DbAuraData>& outAuras);
    bool loadQuests(Guid guid, std::vector<DbQuestData>& outQuests);
    bool loadStatInvestments(Guid guid, std::vector<DbStatInvestment>& outInvestments);
    bool loadWaypoints(Guid guid, std::vector<Guid>& outWaypoints);
    bool loadMail(Guid guid, std::vector<DbMailItem>& outMail);

private:
    MySQLConnector* m_connector = nullptr;

    // Helper: build a formatted query (snprintf-style) up to 4 KB.
    std::string buildQuery(const char* fmt, ...);
};
