#include "database/PlayerDb.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void PlayerDb::setConnector(MySQLConnector* connector) {
    m_connector = connector;
}

std::string PlayerDb::buildQuery(const char* fmt, ...) {
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return std::string(buf);
}

static uint32 toU32(const std::string& s) {
    return static_cast<uint32>(std::strtoul(s.c_str(), nullptr, 10));
}

static float toFloat(const std::string& s) {
    return std::strtof(s.c_str(), nullptr);
}

// ---------------------------------------------------------------------------
// Character list
// ---------------------------------------------------------------------------

bool PlayerDb::getCharacterList(uint32 accountId,
                                std::vector<DbCharacterListEntry>& outList) {
    outList.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT name, class, level, guid, portrait, gender "
        "FROM player WHERE account = '%d';", accountId);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outList.reserve(rs.size());
    for (auto& row : rs) {
        DbCharacterListEntry e;
        e.name        = row["name"];
        e.playerClass = toU32(row["class"]);
        e.level       = toU32(row["level"]);
        e.guid        = toU32(row["guid"]);
        e.portrait    = toU32(row["portrait"]);
        e.gender      = toU32(row["gender"]);
        outList.push_back(std::move(e));
    }
    return true;
}

// ---------------------------------------------------------------------------
// Name check
// ---------------------------------------------------------------------------

bool PlayerDb::isNameTaken(const std::string& name, bool& outTaken) {
    if (!m_connector) return false;

    std::string escaped = m_connector->escapeString(name);
    std::string sql = buildQuery(
        "SELECT EXISTS(SELECT 1 FROM player WHERE name = '%s') AS name_taken;",
        escaped.c_str());

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    if (rs.empty()) {
        outTaken = false;
        return true;
    }
    outTaken = (toU32(rs[0]["name_taken"]) != 0);
    return true;
}

// ---------------------------------------------------------------------------
// Max GUID
// ---------------------------------------------------------------------------

bool PlayerDb::getMaxGuid(Guid& outGuid) {
    if (!m_connector) return false;

    ResultSet rs;
    if (!m_connector->query("SELECT MAX(guid) FROM player;", rs))
        return false;

    if (rs.empty() || rs[0].begin()->second.empty()) {
        outGuid = 0;
        return true;
    }
    outGuid = toU32(rs[0].begin()->second);
    return true;
}

// ---------------------------------------------------------------------------
// Full player load
// ---------------------------------------------------------------------------

bool PlayerDb::loadPlayer(Guid guid, DbPlayerRecord& outRecord) {
    if (!m_connector) return false;

    // Main player row.  The original query joins against an account/play-time
    // table and checks GM/admin flags.
    std::string sql = buildQuery(
        "SELECT p.guid, p.account, p.name, p.class, p.gender, p.portrait, "
        "p.level, p.hp_pct, p.mp_pct, p.xp, p.money, "
        "p.position_x, p.position_y, p.orientation, p.map, "
        "p.home_map, p.home_x, p.home_y, "
        "p.logout_time, p.last_death_time, "
        "p.pvp_points, p.pk_count, p.pvp_flag, "
        "p.progression, p.num_invested_spells, p.combat_rating, "
        "IFNULL(apt.timeSecs, 0) AS timePlayed, "
        "IFNULL(a.isGM, 0) AS isGM, "
        "IFNULL(a.isAdmin, 0) AS isAdmin "
        "FROM player p "
        "LEFT JOIN account_play_time apt ON apt.account = p.account "
        "LEFT JOIN account a ON a.id = p.account "
        "WHERE p.guid = '%u';", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    if (rs.empty()) {
        LOG_ERROR("PlayerDb::loadPlayer - No player found with guid %u", guid);
        return false;
    }

    auto& row = rs[0];

    outRecord.guid              = toU32(row["guid"]);
    outRecord.account           = toU32(row["account"]);
    outRecord.name              = row["name"];
    outRecord.playerClass       = toU32(row["class"]);
    outRecord.gender            = toU32(row["gender"]);
    outRecord.portrait          = toU32(row["portrait"]);
    outRecord.level             = toU32(row["level"]);
    outRecord.hpPct             = toFloat(row["hp_pct"]);
    outRecord.mpPct             = toFloat(row["mp_pct"]);
    outRecord.xp                = toU32(row["xp"]);
    outRecord.money             = toU32(row["money"]);
    outRecord.positionX         = toFloat(row["position_x"]);
    outRecord.positionY         = toFloat(row["position_y"]);
    outRecord.orientation       = toFloat(row["orientation"]);
    outRecord.map               = toU32(row["map"]);
    outRecord.homeMap           = toU32(row["home_map"]);
    outRecord.homeX             = toFloat(row["home_x"]);
    outRecord.homeY             = toFloat(row["home_y"]);
    outRecord.logoutTime        = row["logout_time"];
    outRecord.lastDeathTime     = row["last_death_time"];
    outRecord.pvpPoints         = toU32(row["pvp_points"]);
    outRecord.pkCount           = toU32(row["pk_count"]);
    outRecord.pvpFlag           = toU32(row["pvp_flag"]);
    outRecord.progression       = toU32(row["progression"]);
    outRecord.numInvestedSpells = toU32(row["num_invested_spells"]);
    outRecord.combatRating      = toU32(row["combat_rating"]);
    outRecord.timePlayed        = toU32(row["timePlayed"]);
    outRecord.isGM              = (toU32(row["isGM"]) != 0);
    outRecord.isAdmin           = (toU32(row["isAdmin"]) != 0);

    // Load all sub-records
    bool ok = true;
    ok &= loadInventory(guid, outRecord.inventory);
    ok &= loadEquipment(guid, outRecord.equipment);
    ok &= loadBank(guid, outRecord.bank);
    ok &= loadSpells(guid, outRecord.spells);
    ok &= loadSpellCooldowns(guid, outRecord.spellCooldowns);
    ok &= loadAuras(guid, outRecord.auras);
    ok &= loadQuests(guid, outRecord.quests);
    ok &= loadStatInvestments(guid, outRecord.statInvestments);
    ok &= loadWaypoints(guid, outRecord.waypoints);
    ok &= loadMail(guid, outRecord.mail);

    if (!ok)
        LOG_WARN("PlayerDb::loadPlayer - Some sub-records failed to load for guid %u", guid);

    return true;
}

// ---------------------------------------------------------------------------
// Auth tokens
// ---------------------------------------------------------------------------

bool PlayerDb::validateAuthToken(const std::string& token, DbAuthToken& outAuth) {
    if (!m_connector) return false;

    std::string escaped = m_connector->escapeString(token);
    std::string sql = buildQuery(
        "SELECT user_id, token FROM auth_tokens "
        "WHERE token = '%s' AND created_at > NOW() - INTERVAL 60 SECOND",
        escaped.c_str());

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    if (rs.empty()) {
        outAuth.valid = false;
        return true;
    }

    outAuth.userId = toU32(rs[0]["user_id"]);
    outAuth.token  = rs[0]["token"];
    outAuth.valid  = true;
    return true;
}

// ---------------------------------------------------------------------------
// Login credentials
// ---------------------------------------------------------------------------

bool PlayerDb::validateLogin(const std::string& username, const std::string& password, uint32& outAccountId) {
    if (!m_connector) return false;

    std::string escapedUser = m_connector->escapeString(username);
    std::string sql = buildQuery(
        "SELECT id, password, banned FROM accounts WHERE username = '%s'",
        escapedUser.c_str());

    ResultSet rs;
    if (!m_connector->query(sql, rs) || rs.empty())
        return false;

    // Plaintext password comparison
    std::string storedPass = rs[0]["password"];
    if (storedPass != password)
        return false;

    // Check banned
    if (toU32(rs[0]["banned"]) != 0)
        return false;

    outAccountId = toU32(rs[0]["id"]);
    return true;
}

// ---------------------------------------------------------------------------
// Inventory
// ---------------------------------------------------------------------------

bool PlayerDb::loadInventory(Guid guid, std::vector<DbItemData>& outItems) {
    outItems.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT slot, entry, affix, affix_score, gem_1, gem_2, gem_3, gem_4, "
        "enchant_level, soulbound, count, durability "
        "FROM player_inventory WHERE guid = '%u' ORDER BY slot DESC;", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outItems.reserve(rs.size());
    for (auto& row : rs) {
        DbItemData item;
        item.slot         = toU32(row["slot"]);
        item.entry        = toU32(row["entry"]);
        item.affix        = toU32(row["affix"]);
        item.affixScore   = toU32(row["affix_score"]);
        item.gem1         = toU32(row["gem_1"]);
        item.gem2         = toU32(row["gem_2"]);
        item.gem3         = toU32(row["gem_3"]);
        item.gem4         = toU32(row["gem_4"]);
        item.enchantLevel = toU32(row["enchant_level"]);
        item.soulbound    = toU32(row["soulbound"]);
        item.count        = toU32(row["count"]);
        item.durability   = toU32(row["durability"]);
        outItems.push_back(item);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Equipment
// ---------------------------------------------------------------------------

bool PlayerDb::loadEquipment(Guid guid, std::vector<DbEquipmentData>& outItems) {
    outItems.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT entry, affix, affix_score, gem_1, gem_2, gem_3, gem_4, "
        "enchant_level, soulbound, durability "
        "FROM player_equipment WHERE guid = '%u';", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outItems.reserve(rs.size());
    for (auto& row : rs) {
        DbEquipmentData item;
        item.entry        = toU32(row["entry"]);
        item.affix        = toU32(row["affix"]);
        item.affixScore   = toU32(row["affix_score"]);
        item.gem1         = toU32(row["gem_1"]);
        item.gem2         = toU32(row["gem_2"]);
        item.gem3         = toU32(row["gem_3"]);
        item.gem4         = toU32(row["gem_4"]);
        item.enchantLevel = toU32(row["enchant_level"]);
        item.soulbound    = toU32(row["soulbound"]);
        item.durability   = toU32(row["durability"]);
        outItems.push_back(item);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Bank
// ---------------------------------------------------------------------------

bool PlayerDb::loadBank(Guid guid, std::vector<DbItemData>& outItems) {
    outItems.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT slot, entry, affix, affix_score, gem_1, gem_2, gem_3, gem_4, "
        "enchant_level, soulbound, count, durability "
        "FROM player_bank WHERE guid = '%u' ORDER BY slot DESC;", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outItems.reserve(rs.size());
    for (auto& row : rs) {
        DbItemData item;
        item.slot         = toU32(row["slot"]);
        item.entry        = toU32(row["entry"]);
        item.affix        = toU32(row["affix"]);
        item.affixScore   = toU32(row["affix_score"]);
        item.gem1         = toU32(row["gem_1"]);
        item.gem2         = toU32(row["gem_2"]);
        item.gem3         = toU32(row["gem_3"]);
        item.gem4         = toU32(row["gem_4"]);
        item.enchantLevel = toU32(row["enchant_level"]);
        item.soulbound    = toU32(row["soulbound"]);
        item.count        = toU32(row["count"]);
        item.durability   = toU32(row["durability"]);
        outItems.push_back(item);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Spells
// ---------------------------------------------------------------------------

bool PlayerDb::loadSpells(Guid guid, std::vector<DbSpellData>& outSpells) {
    outSpells.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT spell_id, level FROM player_spell WHERE guid = '%u';", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outSpells.reserve(rs.size());
    for (auto& row : rs) {
        DbSpellData spell;
        spell.spellId = toU32(row["spell_id"]);
        spell.level   = toU32(row["level"]);
        outSpells.push_back(spell);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Spell cooldowns
// ---------------------------------------------------------------------------

bool PlayerDb::loadSpellCooldowns(Guid guid,
                                  std::vector<DbSpellCooldownData>& outCooldowns) {
    outCooldowns.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT spell_id, start_date, end_date "
        "FROM player_spell_cooldown WHERE guid = '%u';", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outCooldowns.reserve(rs.size());
    for (auto& row : rs) {
        DbSpellCooldownData cd;
        cd.spellId   = toU32(row["spell_id"]);
        cd.startDate = row["start_date"];
        cd.endDate   = row["end_date"];
        outCooldowns.push_back(std::move(cd));
    }
    return true;
}

// ---------------------------------------------------------------------------
// Auras
// ---------------------------------------------------------------------------

bool PlayerDb::loadAuras(Guid guid, std::vector<DbAuraData>& outAuras) {
    outAuras.clear();
    if (!m_connector) return false;

    // The original binary stores 3 effect ticks per aura row with columns
    // for each effect index (base_amount_0..2, tick_amount_0..2, ticks_left_0..2).
    std::string sql = buildQuery(
        "SELECT spell_id, caster_guid, duration, max_duration, stacks, "
        "base_amount_0, tick_amount_0, ticks_left_0, "
        "base_amount_1, tick_amount_1, ticks_left_1, "
        "base_amount_2, tick_amount_2, ticks_left_2 "
        "FROM player_aura WHERE guid = '%u';", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outAuras.reserve(rs.size());
    for (auto& row : rs) {
        DbAuraData aura;
        aura.spellId     = toU32(row["spell_id"]);
        aura.casterGuid  = toU32(row["caster_guid"]);
        aura.duration    = toU32(row["duration"]);
        aura.maxDuration = toU32(row["max_duration"]);
        aura.stacks      = toU32(row["stacks"]);

        for (int i = 0; i < MAX_AURA_EFFECTS; ++i) {
            std::string idx = std::to_string(i);
            aura.effects[i].baseAmount = toU32(row["base_amount_" + idx]);
            aura.effects[i].tickAmount = toU32(row["tick_amount_" + idx]);
            aura.effects[i].ticksLeft  = toU32(row["ticks_left_"  + idx]);
        }

        outAuras.push_back(aura);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Quests
// ---------------------------------------------------------------------------

bool PlayerDb::loadQuests(Guid guid, std::vector<DbQuestData>& outQuests) {
    outQuests.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT quest, rewarded, objective_count1, objective_count2, "
        "objective_count3, objective_count4 "
        "FROM player_quest_status WHERE guid = '%u';", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outQuests.reserve(rs.size());
    for (auto& row : rs) {
        DbQuestData q;
        q.questId         = toU32(row["quest"]);
        q.rewarded        = toU32(row["rewarded"]);
        q.objectiveCount1 = toU32(row["objective_count1"]);
        q.objectiveCount2 = toU32(row["objective_count2"]);
        q.objectiveCount3 = toU32(row["objective_count3"]);
        q.objectiveCount4 = toU32(row["objective_count4"]);
        outQuests.push_back(q);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Stat investments
// ---------------------------------------------------------------------------

bool PlayerDb::loadStatInvestments(Guid guid,
                                   std::vector<DbStatInvestment>& outInvestments) {
    outInvestments.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT stat_type, amount FROM player_stat_invest WHERE guid = '%u';",
        guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outInvestments.reserve(rs.size());
    for (auto& row : rs) {
        DbStatInvestment si;
        si.statType = toU32(row["stat_type"]);
        si.amount   = toU32(row["amount"]);
        outInvestments.push_back(si);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Waypoints
// ---------------------------------------------------------------------------

bool PlayerDb::loadWaypoints(Guid guid, std::vector<Guid>& outWaypoints) {
    outWaypoints.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT object_dbguid FROM player_waypoints WHERE player_guid = '%u';",
        guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outWaypoints.reserve(rs.size());
    for (auto& row : rs) {
        outWaypoints.push_back(toU32(row["object_dbguid"]));
    }
    return true;
}

// ---------------------------------------------------------------------------
// Mail
// ---------------------------------------------------------------------------

bool PlayerDb::loadMail(Guid guid, std::vector<DbMailItem>& outMail) {
    outMail.clear();
    if (!m_connector) return false;

    std::string sql = buildQuery(
        "SELECT entry, affix, affix_score, count, gem_1, gem_2, gem_3, gem_4, "
        "enchant_level, soulbound, durability "
        "FROM player_mail WHERE guid = '%u';", guid);

    ResultSet rs;
    if (!m_connector->query(sql, rs))
        return false;

    outMail.reserve(rs.size());
    for (auto& row : rs) {
        DbMailItem mi;
        mi.entry        = toU32(row["entry"]);
        mi.affix        = toU32(row["affix"]);
        mi.affixScore   = toU32(row["affix_score"]);
        mi.count        = toU32(row["count"]);
        mi.gem1         = toU32(row["gem_1"]);
        mi.gem2         = toU32(row["gem_2"]);
        mi.gem3         = toU32(row["gem_3"]);
        mi.gem4         = toU32(row["gem_4"]);
        mi.enchantLevel = toU32(row["enchant_level"]);
        mi.soulbound    = toU32(row["soulbound"]);
        mi.durability   = toU32(row["durability"]);
        outMail.push_back(mi);
    }
    return true;
}
