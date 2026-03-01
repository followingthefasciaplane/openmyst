#include "game/ServerPlayer.h"
#include "network/Session.h"
#include "network/GamePacket.h"
#include "game/GameCache.h"
#include "database/DatabaseMgr.h"
#include "core/Log.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>

// Resolved at link time by the server module
extern GameCache& getGameCache();

// Static empty item for returning references to non-existent slots
const ItemInstance ServerPlayer::s_emptyItem;

// ---------------------------------------------------------------------------
//  Constructor / Destructor
// ---------------------------------------------------------------------------

ServerPlayer::ServerPlayer() {
    // Inventory, equipment, and bank arrays are zero-initialized by default.
    // Vectors and sets are default-constructed empty.
}

ServerPlayer::~ServerPlayer() {
    m_session = nullptr;
}

// ---------------------------------------------------------------------------
//  XP / Level
// ---------------------------------------------------------------------------

void ServerPlayer::addXp(uint32 amount) {
    m_xp += amount;

    // Check for level-ups against the ExpLevel table
    for (;;) {
        const ExpLevel* expLevel = getGameCache().getExpLevel(m_level);
        if (!expLevel) break;           // no data for current level -- cap reached
        if (expLevel->exp == 0) break;  // sentinel: max level
        if (m_xp < expLevel->exp) break;

        m_xp -= expLevel->exp;
        m_level++;

        LOG_INFO("Player %s leveled up to %u", m_name.c_str(), (unsigned)m_level);
        recalculateStats();
    }
}

// ---------------------------------------------------------------------------
//  Money
// ---------------------------------------------------------------------------

void ServerPlayer::addMoney(int32 amount) {
    int64 result = static_cast<int64>(m_money) + amount;
    if (result < 0) result = 0;
    m_money = static_cast<uint32>(result);
}

// ---------------------------------------------------------------------------
//  Home position
// ---------------------------------------------------------------------------

void ServerPlayer::setHome(uint32 map, float x, float y) {
    m_homeMap = map;
    m_homeX = x;
    m_homeY = y;
}

// ---------------------------------------------------------------------------
//  Inventory
// ---------------------------------------------------------------------------

const ItemInstance& ServerPlayer::getInventoryItem(uint32 slot) const {
    if (slot >= MAX_INVENTORY_SLOTS) return s_emptyItem;
    return m_inventory[slot];
}

bool ServerPlayer::setInventoryItem(uint32 slot, const ItemInstance& item) {
    if (slot >= MAX_INVENTORY_SLOTS) return false;
    m_inventory[slot] = item;
    return true;
}

bool ServerPlayer::addItemToInventory(const ItemInstance& item) {
    // Try to stack with an existing item of the same entry first
    if (item.entry != 0) {
        const ItemTemplate* tmpl = getGameCache().getItemTemplate(item.entry);
        uint32 maxStack = tmpl ? tmpl->stackCount : 1;

        if (maxStack > 1) {
            for (uint32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
                if (m_inventory[i].entry == item.entry &&
                    m_inventory[i].count < maxStack) {
                    uint32 space = maxStack - m_inventory[i].count;
                    uint32 toAdd = std::min(space, item.count);
                    m_inventory[i].count += toAdd;
                    if (toAdd >= item.count) return true;
                    // If we couldn't fit everything, fall through to find a free slot
                    // for the remainder. For simplicity, place remainder in a new slot.
                    ItemInstance remainder = item;
                    remainder.count = item.count - toAdd;
                    int freeSlot = findFreeInventorySlot();
                    if (freeSlot < 0) return false;
                    m_inventory[freeSlot] = remainder;
                    return true;
                }
            }
        }
    }

    // Find first free slot
    int slot = findFreeInventorySlot();
    if (slot < 0) return false;

    m_inventory[slot] = item;
    return true;
}

bool ServerPlayer::removeInventoryItem(uint32 slot, uint32 count) {
    if (slot >= MAX_INVENTORY_SLOTS) return false;
    if (m_inventory[slot].isEmpty()) return false;

    if (m_inventory[slot].count <= count) {
        m_inventory[slot].clear();
    } else {
        m_inventory[slot].count -= count;
    }
    return true;
}

int ServerPlayer::findFreeInventorySlot() const {
    for (uint32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (m_inventory[i].isEmpty()) return static_cast<int>(i);
    }
    return -1;
}

bool ServerPlayer::hasItem(uint32 entry, uint32 count) const {
    return countItem(entry) >= count;
}

uint32 ServerPlayer::countItem(uint32 entry) const {
    uint32 total = 0;
    for (uint32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (m_inventory[i].entry == entry) {
            total += m_inventory[i].count;
        }
    }
    // Also check equipment
    for (uint32 i = 0; i < MAX_EQUIPMENT_SLOTS; i++) {
        if (m_equipment[i].entry == entry) {
            total += m_equipment[i].count;
        }
    }
    return total;
}

// ---------------------------------------------------------------------------
//  Equipment
// ---------------------------------------------------------------------------

const ItemInstance& ServerPlayer::getEquipment(uint32 slot) const {
    if (slot >= MAX_EQUIPMENT_SLOTS) return s_emptyItem;
    return m_equipment[slot];
}

bool ServerPlayer::equipItem(uint32 inventorySlot) {
    if (inventorySlot >= MAX_INVENTORY_SLOTS) return false;

    const ItemInstance& invItem = m_inventory[inventorySlot];
    if (invItem.isEmpty()) return false;

    const ItemTemplate* tmpl = getGameCache().getItemTemplate(invItem.entry);
    if (!tmpl) return false;
    if (tmpl->equipType == 0) return false;  // EquipType::None -- not equippable

    // Determine the equipment slot from the item's equip type
    uint32 equipSlot = tmpl->equipType - 1;  // EquipType enum is 1-based, slots are 0-based
    if (equipSlot >= MAX_EQUIPMENT_SLOTS) return false;

    // Check level requirement
    if (tmpl->requiredLevel > m_level) {
        LOG_WARN("Player %s cannot equip item %u: requires level %u (player is %u)",
                 m_name.c_str(), invItem.entry, tmpl->requiredLevel, (unsigned)m_level);
        return false;
    }

    // Check class requirement
    if (tmpl->requiredClass != 0 && tmpl->requiredClass != m_classId) {
        LOG_WARN("Player %s cannot equip item %u: wrong class", m_name.c_str(), invItem.entry);
        return false;
    }

    // Swap: move currently equipped item (if any) to the inventory slot
    ItemInstance oldEquipped = m_equipment[equipSlot];
    m_equipment[equipSlot] = invItem;

    if (!oldEquipped.isEmpty()) {
        m_inventory[inventorySlot] = oldEquipped;
    } else {
        m_inventory[inventorySlot].clear();
    }

    recalculateStats();
    return true;
}

bool ServerPlayer::unequipItem(uint32 equipSlot) {
    if (equipSlot >= MAX_EQUIPMENT_SLOTS) return false;
    if (m_equipment[equipSlot].isEmpty()) return false;

    int freeSlot = findFreeInventorySlot();
    if (freeSlot < 0) {
        LOG_WARN("Player %s cannot unequip: inventory full", m_name.c_str());
        return false;
    }

    m_inventory[freeSlot] = m_equipment[equipSlot];
    m_equipment[equipSlot].clear();

    recalculateStats();
    return true;
}

// ---------------------------------------------------------------------------
//  Bank
// ---------------------------------------------------------------------------

const ItemInstance& ServerPlayer::getBankItem(uint32 slot) const {
    if (slot >= MAX_BANK_SLOTS) return s_emptyItem;
    return m_bank[slot];
}

bool ServerPlayer::setBankItem(uint32 slot, const ItemInstance& item) {
    if (slot >= MAX_BANK_SLOTS) return false;
    m_bank[slot] = item;
    return true;
}

bool ServerPlayer::moveToBankSlot(uint32 invSlot, uint32 bankSlot) {
    if (invSlot >= MAX_INVENTORY_SLOTS) return false;
    if (bankSlot >= MAX_BANK_SLOTS) return false;
    if (m_inventory[invSlot].isEmpty()) return false;

    // Swap inventory item with bank slot (bank slot may be empty or occupied)
    ItemInstance temp = m_bank[bankSlot];
    m_bank[bankSlot] = m_inventory[invSlot];

    if (!temp.isEmpty()) {
        m_inventory[invSlot] = temp;
    } else {
        m_inventory[invSlot].clear();
    }
    return true;
}

bool ServerPlayer::moveFromBankSlot(uint32 bankSlot, uint32 invSlot) {
    if (bankSlot >= MAX_BANK_SLOTS) return false;
    if (invSlot >= MAX_INVENTORY_SLOTS) return false;
    if (m_bank[bankSlot].isEmpty()) return false;

    // Swap bank item with inventory slot (inventory slot may be empty or occupied)
    ItemInstance temp = m_inventory[invSlot];
    m_inventory[invSlot] = m_bank[bankSlot];

    if (!temp.isEmpty()) {
        m_bank[bankSlot] = temp;
    } else {
        m_bank[bankSlot].clear();
    }
    return true;
}

// ---------------------------------------------------------------------------
//  Spells
// ---------------------------------------------------------------------------

bool ServerPlayer::learnSpell(uint32 spellId, uint32 level) {
    // Check if already known -- if so, update level
    for (auto& spell : m_spells) {
        if (spell.spellId == spellId) {
            if (level > spell.level) {
                spell.level = level;
            }
            return true;
        }
    }

    PlayerSpell newSpell;
    newSpell.spellId = spellId;
    newSpell.level = level;
    m_spells.push_back(newSpell);
    return true;
}

bool ServerPlayer::unlearnSpell(uint32 spellId) {
    auto it = std::find_if(m_spells.begin(), m_spells.end(),
        [spellId](const PlayerSpell& s) { return s.spellId == spellId; });
    if (it == m_spells.end()) return false;
    m_spells.erase(it);
    return true;
}

bool ServerPlayer::hasSpell(uint32 spellId) const {
    for (const auto& spell : m_spells) {
        if (spell.spellId == spellId) return true;
    }
    return false;
}

const PlayerSpell* ServerPlayer::getSpell(uint32 spellId) const {
    for (const auto& spell : m_spells) {
        if (spell.spellId == spellId) return &spell;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
//  Cooldowns
// ---------------------------------------------------------------------------

bool ServerPlayer::isSpellOnCooldown(uint32 spellId) const {
    int64 now = static_cast<int64>(std::time(nullptr));
    for (const auto& cd : m_cooldowns) {
        if (cd.spellId == spellId) {
            return now < cd.endDate;
        }
    }
    return false;
}

void ServerPlayer::addSpellCooldown(uint32 spellId, int64 startDate, int64 endDate) {
    // Replace existing cooldown for this spell if present
    for (auto& cd : m_cooldowns) {
        if (cd.spellId == spellId) {
            cd.startDate = startDate;
            cd.endDate = endDate;
            return;
        }
    }

    SpellCooldown newCd;
    newCd.spellId = spellId;
    newCd.startDate = startDate;
    newCd.endDate = endDate;
    m_cooldowns.push_back(newCd);
}

void ServerPlayer::removeSpellCooldown(uint32 spellId) {
    m_cooldowns.erase(
        std::remove_if(m_cooldowns.begin(), m_cooldowns.end(),
            [spellId](const SpellCooldown& cd) { return cd.spellId == spellId; }),
        m_cooldowns.end());
}

// ---------------------------------------------------------------------------
//  Quests
// ---------------------------------------------------------------------------

bool ServerPlayer::acceptQuest(uint32 questEntry) {
    // Check if already active or completed
    if (hasActiveQuest(questEntry)) return false;

    const QuestTemplate* tmpl = getGameCache().getQuestTemplate(questEntry);
    if (!tmpl) {
        LOG_WARN("Player %s tried to accept unknown quest %u", m_name.c_str(), questEntry);
        return false;
    }

    // Check level requirement
    if (tmpl->minLevel > m_level) {
        LOG_WARN("Player %s too low level for quest %u (need %u, have %u)",
                 m_name.c_str(), questEntry, tmpl->minLevel, (unsigned)m_level);
        return false;
    }

    // Check prerequisites
    for (uint32 i = 0; i < MAX_QUEST_PREREQS; i++) {
        if (tmpl->prevQuest[i] != 0 && !hasCompletedQuest(tmpl->prevQuest[i])) {
            LOG_WARN("Player %s missing prerequisite quest %u for quest %u",
                     m_name.c_str(), tmpl->prevQuest[i], questEntry);
            return false;
        }
    }

    QuestStatus status;
    status.questEntry = questEntry;
    status.rewarded = 0;
    for (uint32 i = 0; i < MAX_QUEST_OBJECTIVES; i++) {
        status.objectiveCount[i] = 0;
    }

    m_quests.push_back(status);

    // Give provided item if any
    if (tmpl->providedItem != 0) {
        ItemInstance providedItem;
        providedItem.entry = tmpl->providedItem;
        providedItem.count = 1;
        addItemToInventory(providedItem);
    }

    return true;
}

bool ServerPlayer::completeQuest(uint32 questEntry, uint32 rewardChoice) {
    QuestStatus* status = getQuestStatus(questEntry);
    if (!status) return false;
    if (status->rewarded != 0) return false;  // Already completed and rewarded

    const QuestTemplate* tmpl = getGameCache().getQuestTemplate(questEntry);
    if (!tmpl) return false;

    // Verify all objectives are met
    if (!status->isComplete(*tmpl)) {
        LOG_WARN("Player %s tried to complete quest %u but objectives are not met",
                 m_name.c_str(), questEntry);
        return false;
    }

    // Mark as rewarded
    status->rewarded = 1;

    // Grant fixed rewards
    for (uint32 i = 0; i < MAX_QUEST_REWARDS; i++) {
        if (tmpl->rewItem[i] != 0) {
            ItemInstance reward;
            reward.entry = tmpl->rewItem[i];
            reward.count = tmpl->rewItemCount[i] > 0 ? tmpl->rewItemCount[i] : 1;
            addItemToInventory(reward);
        }
    }

    // Grant chosen reward
    if (rewardChoice < MAX_QUEST_REWARD_CHOICES && tmpl->rewChoiceItem[rewardChoice] != 0) {
        ItemInstance chosenReward;
        chosenReward.entry = tmpl->rewChoiceItem[rewardChoice];
        chosenReward.count = tmpl->rewChoiceCount[rewardChoice] > 0
                             ? tmpl->rewChoiceCount[rewardChoice] : 1;
        addItemToInventory(chosenReward);
    }

    // Grant money and XP rewards
    if (tmpl->rewMoney > 0) {
        addMoney(static_cast<int32>(tmpl->rewMoney));
    }
    if (tmpl->rewXp > 0) {
        addXp(tmpl->rewXp);
    }

    LOG_INFO("Player %s completed quest %u", m_name.c_str(), questEntry);
    return true;
}

bool ServerPlayer::hasCompletedQuest(uint32 questEntry) const {
    for (const auto& q : m_quests) {
        if (q.questEntry == questEntry && q.rewarded != 0) return true;
    }
    return false;
}

bool ServerPlayer::hasActiveQuest(uint32 questEntry) const {
    for (const auto& q : m_quests) {
        if (q.questEntry == questEntry) return true;
    }
    return false;
}

QuestStatus* ServerPlayer::getQuestStatus(uint32 questEntry) {
    for (auto& q : m_quests) {
        if (q.questEntry == questEntry) return &q;
    }
    return nullptr;
}

void ServerPlayer::updateQuestObjective(uint32 questEntry, uint32 objectiveIdx, uint32 count) {
    if (objectiveIdx >= MAX_QUEST_OBJECTIVES) return;

    QuestStatus* status = getQuestStatus(questEntry);
    if (!status) return;
    if (status->rewarded != 0) return;  // Already completed

    status->objectiveCount[objectiveIdx] += count;

    // Clamp to the required count from the template
    const QuestTemplate* tmpl = getGameCache().getQuestTemplate(questEntry);
    if (tmpl && tmpl->reqCount[objectiveIdx] > 0) {
        if (status->objectiveCount[objectiveIdx] > tmpl->reqCount[objectiveIdx]) {
            status->objectiveCount[objectiveIdx] = tmpl->reqCount[objectiveIdx];
        }
    }
}

// ---------------------------------------------------------------------------
//  Stat Investments
// ---------------------------------------------------------------------------

void ServerPlayer::investStat(uint32 statType, int32 amount) {
    for (auto& inv : m_statInvestments) {
        if (inv.statType == statType) {
            inv.amount += amount;
            recalculateStats();
            return;
        }
    }

    StatInvestment newInv;
    newInv.statType = statType;
    newInv.amount = amount;
    m_statInvestments.push_back(newInv);
    recalculateStats();
}

int32 ServerPlayer::getStatInvestment(uint32 statType) const {
    for (const auto& inv : m_statInvestments) {
        if (inv.statType == statType) return inv.amount;
    }
    return 0;
}

// ---------------------------------------------------------------------------
//  Waypoints
// ---------------------------------------------------------------------------

bool ServerPlayer::hasDiscoveredWaypoint(uint32 objectGuid) const {
    return m_discoveredWaypoints.count(objectGuid) > 0;
}

void ServerPlayer::discoverWaypoint(uint32 objectGuid) {
    m_discoveredWaypoints.insert(objectGuid);
}

// ---------------------------------------------------------------------------
//  Mail
// ---------------------------------------------------------------------------

void ServerPlayer::addMail(const ItemInstance& item) {
    m_mail.push_back(item);
}

// ---------------------------------------------------------------------------
//  Ignore List
// ---------------------------------------------------------------------------

bool ServerPlayer::isIgnoring(uint32 targetAccountId) const {
    return m_ignoreList.count(targetAccountId) > 0;
}

void ServerPlayer::addIgnore(uint32 targetAccountId) {
    m_ignoreList.insert(targetAccountId);
}

void ServerPlayer::removeIgnore(uint32 targetAccountId) {
    m_ignoreList.erase(targetAccountId);
}

// ---------------------------------------------------------------------------
//  Packet Sending
// ---------------------------------------------------------------------------

void ServerPlayer::sendPacket(const GamePacket& packet) {
    if (m_session) {
        m_session->send(packet);
    }
}

// ---------------------------------------------------------------------------
//  Stat Recalculation
// ---------------------------------------------------------------------------

void ServerPlayer::recalculateStats() {
    // Start from class base stats for current level
    const ClassStats* baseStats = getGameCache().getClassStats(m_classId, m_level);
    if (!baseStats) {
        LOG_WARN("No class stats found for class %u level %u", (unsigned)m_classId, (unsigned)m_level);
        return;
    }

    // Set base stats from class table
    setStat(StatType::Strength,     static_cast<int32>(baseStats->strength));
    setStat(StatType::Agility,      static_cast<int32>(baseStats->agility));
    setStat(StatType::Willpower,    static_cast<int32>(baseStats->willpower));
    setStat(StatType::Intelligence, static_cast<int32>(baseStats->intelligence));
    setStat(StatType::Courage,      static_cast<int32>(baseStats->courage));
    setStat(StatType::Hp,           static_cast<int32>(baseStats->hp));
    setStat(StatType::Mana,         static_cast<int32>(baseStats->mana));

    m_maxHealth = static_cast<int32>(baseStats->hp);
    m_maxMana   = static_cast<int32>(baseStats->mana);

    // Apply stat investments
    for (const auto& inv : m_statInvestments) {
        if (inv.statType < static_cast<uint32>(StatType::Count)) {
            StatType st = static_cast<StatType>(inv.statType);
            setStat(st, getStat(st) + inv.amount);

            // HP and Mana investments affect max pools directly
            if (st == StatType::Hp) {
                m_maxHealth += inv.amount;
            } else if (st == StatType::Mana) {
                m_maxMana += inv.amount;
            }
        }
    }

    // Apply equipment bonuses
    int32 totalArmor = 0;
    for (uint32 i = 0; i < MAX_EQUIPMENT_SLOTS; i++) {
        if (m_equipment[i].isEmpty()) continue;

        const ItemTemplate* itemTmpl = getGameCache().getItemTemplate(m_equipment[i].entry);
        if (!itemTmpl) continue;

        // Add item stats
        for (uint32 s = 0; s < MAX_ITEM_STATS; s++) {
            if (itemTmpl->stats[s].type == 0) continue;
            if (itemTmpl->stats[s].type < static_cast<uint32>(StatType::Count)) {
                StatType st = static_cast<StatType>(itemTmpl->stats[s].type);
                setStat(st, getStat(st) + itemTmpl->stats[s].value);

                if (st == StatType::Hp) {
                    m_maxHealth += itemTmpl->stats[s].value;
                } else if (st == StatType::Mana) {
                    m_maxMana += itemTmpl->stats[s].value;
                }
            }
        }

        // Add affix stats if present
        if (m_equipment[i].affix != 0) {
            const AffixTemplate* affixTmpl = getGameCache().getAffixTemplate(m_equipment[i].affix);
            if (affixTmpl) {
                for (uint32 s = 0; s < MAX_AFFIX_STATS; s++) {
                    if (affixTmpl->stats[s].type == 0) continue;
                    if (affixTmpl->stats[s].type < static_cast<uint32>(StatType::Count)) {
                        StatType st = static_cast<StatType>(affixTmpl->stats[s].type);
                        setStat(st, getStat(st) + affixTmpl->stats[s].value);

                        if (st == StatType::Hp) {
                            m_maxHealth += affixTmpl->stats[s].value;
                        } else if (st == StatType::Mana) {
                            m_maxMana += affixTmpl->stats[s].value;
                        }
                    }
                }
            }
        }

        // Add gem stats
        uint32 gemIds[MAX_ITEM_SOCKETS] = {
            m_equipment[i].gem1, m_equipment[i].gem2,
            m_equipment[i].gem3, m_equipment[i].gem4
        };
        for (uint32 g = 0; g < MAX_ITEM_SOCKETS; g++) {
            if (gemIds[g] == 0) continue;
            const GemTemplate* gemTmpl = getGameCache().getGemTemplate(gemIds[g]);
            if (!gemTmpl) continue;

            if (gemTmpl->stat1 != 0 && gemTmpl->stat1 < static_cast<uint32>(StatType::Count)) {
                StatType st = static_cast<StatType>(gemTmpl->stat1);
                setStat(st, getStat(st) + gemTmpl->stat1Amount);
                if (st == StatType::Hp) m_maxHealth += gemTmpl->stat1Amount;
                else if (st == StatType::Mana) m_maxMana += gemTmpl->stat1Amount;
            }
            if (gemTmpl->stat2 != 0 && gemTmpl->stat2 < static_cast<uint32>(StatType::Count)) {
                StatType st = static_cast<StatType>(gemTmpl->stat2);
                setStat(st, getStat(st) + gemTmpl->stat2Amount);
                if (st == StatType::Hp) m_maxHealth += gemTmpl->stat2Amount;
                else if (st == StatType::Mana) m_maxMana += gemTmpl->stat2Amount;
            }
        }

        // Accumulate armor
        totalArmor += static_cast<int32>(itemTmpl->durability);  // armor value from item
    }

    m_armor = totalArmor;

    // Clamp health/mana to new maximums
    if (m_health > m_maxHealth) m_health = m_maxHealth;
    if (m_mana > m_maxMana)     m_mana = m_maxMana;
}

// ---------------------------------------------------------------------------
//  Update
// ---------------------------------------------------------------------------

void ServerPlayer::update(int32 deltaMs) {
    // Call base class update (ticks auras)
    ServerUnit::update(deltaMs);

    // Tick cooldowns -- remove expired ones
    int64 now = static_cast<int64>(std::time(nullptr));
    m_cooldowns.erase(
        std::remove_if(m_cooldowns.begin(), m_cooldowns.end(),
            [now](const SpellCooldown& cd) { return now >= cd.endDate; }),
        m_cooldowns.end());
}

// ---------------------------------------------------------------------------
//  Death
// ---------------------------------------------------------------------------

void ServerPlayer::onDeath(ServerUnit* killer) {
    // Call base class death handling (sets hp to 0, clears combat, removes auras)
    ServerUnit::onDeath(killer);

    m_lastDeathTime = static_cast<int64>(std::time(nullptr));

    // Teleport player to the graveyard for their current map
    const Graveyard* graveyard = getGameCache().getGraveyard(m_mapId);
    if (graveyard) {
        m_mapId = graveyard->respawnMap;
        m_posX = graveyard->respawnX;
        m_posY = graveyard->respawnY;
        LOG_INFO("Player %s died, teleporting to graveyard (map %u, %.1f, %.1f)",
                 m_name.c_str(), graveyard->respawnMap, graveyard->respawnX, graveyard->respawnY);
    } else {
        // Fallback: teleport to home position
        m_mapId = m_homeMap;
        m_posX = m_homeX;
        m_posY = m_homeY;
        LOG_WARN("Player %s died but no graveyard found for map %u, using home position",
                 m_name.c_str(), m_mapId);
    }
}

// ---------------------------------------------------------------------------
//  Database Persistence
// ---------------------------------------------------------------------------

void ServerPlayer::saveToDB() {
    auto& db = DatabaseMgr::instance().getPlayerConnection();
    char buf[2048];

    // -- Base character data (REPLACE INTO player) --
    // Column list from binary at 0x00587638
    float hpPct = m_maxHealth > 0 ? static_cast<float>(m_health) / m_maxHealth : 1.0f;
    float mpPct = m_maxMana > 0 ? static_cast<float>(m_mana) / m_maxMana : 1.0f;
    m_logoutTime = static_cast<int64>(std::time(nullptr));

    snprintf(buf, sizeof(buf),
        "REPLACE INTO `player` ("
        "`guid`, `account`, `name`, `class`, `gender`, `portrait`, `level`, "
        "`hp_pct`, `mp_pct`, `xp`, `money`, `position_x`, `position_y`, "
        "`orientation`, `map`, `home_map`, `home_x`, `home_y`, "
        "`logout_time`, `last_death_time`, `pvp_points`, `pk_count`, "
        "`pvp_flag`, `progression`, `num_invested_spells`, `combat_rating`, "
        "`time_played`, `is_gm`, `is_admin`"
        ") VALUES ("
        "'%u', '%u', '%s', '%u', '%u', '%u', '%u', "
        "'%f', '%f', '%u', '%u', '%f', '%f', "
        "'%f', '%u', '%u', '%f', '%f', "
        "'%lld', '%lld', '%d', '%d', "
        "'%u', '%u', '%u', '%u', "
        "'%u', '%d', '%d')",
        m_guid, m_accountId, db.escapeString(m_name).c_str(),
        (unsigned)m_classId, (unsigned)m_gender, (unsigned)m_portrait, (unsigned)m_level,
        hpPct, mpPct, m_xp, m_money, m_posX, m_posY,
        m_orientation, m_mapId, m_homeMap, m_homeX, m_homeY,
        (long long)m_logoutTime, (long long)m_lastDeathTime,
        m_pvpPoints, m_pkCount,
        (unsigned)m_pvpFlag, m_progression, m_numInvestedSpells, m_combatRating,
        m_timePlayed, m_isGM ? 1 : 0, m_isAdmin ? 1 : 0);
    db.execute(buf);

    // -- Played time (from binary at 0x005878d4) --
    snprintf(buf, sizeof(buf),
        "REPLACE INTO `account_played_time` (id, timeSecs) VALUES ('%u', '%u')",
        m_accountId, m_timePlayed);
    db.execute(buf);

    // -- Inventory (DELETE + INSERT per slot) --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_inventory` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (uint32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        const auto& item = m_inventory[i];
        if (item.isEmpty()) continue;
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_inventory` (`guid`, `slot`, `entry`, `affix`, `affix_score`, "
            "`gem1`, `gem2`, `gem3`, `gem4`, `enchant_level`, `count`, `soulbound`, `durability`) "
            "VALUES('%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%d')",
            m_guid, i, item.entry, item.affix, item.affixScore,
            item.gem1, item.gem2, item.gem3, item.gem4,
            item.enchantLevel, item.count, item.soulbound ? 1 : 0, item.durability);
        db.execute(buf);
    }

    // -- Equipment --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_equipment` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (uint32 i = 0; i < MAX_EQUIPMENT_SLOTS; i++) {
        const auto& item = m_equipment[i];
        if (item.isEmpty()) continue;
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_equipment` (`guid`, `slot`, `entry`, `affix`, `affix_score`, "
            "`gem1`, `gem2`, `gem3`, `gem4`, `enchant_level`, `count`, `soulbound`, `durability`) "
            "VALUES('%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%d')",
            m_guid, i, item.entry, item.affix, item.affixScore,
            item.gem1, item.gem2, item.gem3, item.gem4,
            item.enchantLevel, item.count, item.soulbound ? 1 : 0, item.durability);
        db.execute(buf);
    }

    // -- Bank --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_bank` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (uint32 i = 0; i < MAX_BANK_SLOTS; i++) {
        const auto& item = m_bank[i];
        if (item.isEmpty()) continue;
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_bank` (`guid`, `slot`, `entry`, `affix`, `affix_score`, "
            "`gem1`, `gem2`, `gem3`, `gem4`, `enchant_level`, `count`, `soulbound`, `durability`) "
            "VALUES('%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%d')",
            m_guid, i, item.entry, item.affix, item.affixScore,
            item.gem1, item.gem2, item.gem3, item.gem4,
            item.enchantLevel, item.count, item.soulbound ? 1 : 0, item.durability);
        db.execute(buf);
    }

    // -- Spells --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_spell` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (const auto& spell : m_spells) {
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_spell` (`guid`, `spell_id`, `level`) VALUES('%u','%u','%u')",
            m_guid, spell.spellId, spell.level);
        db.execute(buf);
    }

    // -- Cooldowns --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_spell_cooldown` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (const auto& cd : m_cooldowns) {
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_spell_cooldown` (`guid`, `spell_id`, `start_date`, `end_date`) "
            "VALUES('%u','%u','%lld','%lld')",
            m_guid, cd.spellId, (long long)cd.startDate, (long long)cd.endDate);
        db.execute(buf);
    }

    // -- Quests (REPLACE for active, DELETE non-rewarded) --
    snprintf(buf, sizeof(buf),
        "DELETE FROM `player_quest_status` WHERE `guid` = '%u' AND `rewarded` = 0", m_guid);
    db.execute(buf);
    for (const auto& q : m_quests) {
        snprintf(buf, sizeof(buf),
            "REPLACE INTO `player_quest_status` (`guid`, `quest_entry`, `status`, "
            "`obj1_count`, `obj2_count`, `obj3_count`, `obj4_count`, `rewarded`) "
            "VALUES ('%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u')",
            m_guid, q.questEntry, q.rewarded != 0 ? 1u : 0u,
            q.objectiveCount[0], q.objectiveCount[1],
            q.objectiveCount[2], q.objectiveCount[3], q.rewarded);
        db.execute(buf);
    }

    // -- Stat investments --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_stat_invest` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (const auto& inv : m_statInvestments) {
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_stat_invest` (`guid`, `stat_type`, `amount`) "
            "VALUES('%u', '%u', '%d')",
            m_guid, inv.statType, inv.amount);
        db.execute(buf);
    }

    // -- Waypoints --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_waypoints` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (uint32 wp : m_discoveredWaypoints) {
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_waypoints` (`guid`, `object_guid`) VALUES('%u', '%u')",
            m_guid, wp);
        db.execute(buf);
    }

    // -- Ignore list --
    snprintf(buf, sizeof(buf), "DELETE FROM `account_ignore` WHERE `account_id` = '%u'", m_accountId);
    db.execute(buf);
    for (uint32 ignoreId : m_ignoreList) {
        snprintf(buf, sizeof(buf),
            "INSERT INTO `account_ignore` (`account_id`, `target_name`) VALUES('%u','%u')",
            m_accountId, ignoreId);
        db.execute(buf);
    }

    // -- Auras --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_aura` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (const auto& aura : m_auras) {
        if (!aura.isActive()) continue;
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_aura` (`guid`, `caster_guid`, `spell_id`, `miliseconds`, "
            "`m_stackCount`, `m_spellLevel`, `m_casterLevel`, "
            "`m_tickTotal_1`, `m_tickAmount_1`, `m_tickAmountTicked_1`, `m_casterGuid_1`, "
            "`m_numTicks_1`, `m_numTicksCounter_1`, `m_tickTimer_1`, `m_tickIntervalMs_1`, "
            "`m_tickTotal_2`, `m_tickAmount_2`, `m_tickAmountTicked_2`, `m_casterGuid_2`, "
            "`m_numTicks_2`, `m_numTicksCounter_2`, `m_tickTimer_2`, `m_tickIntervalMs_2`, "
            "`m_tickTotal_3`, `m_tickAmount_3`, `m_tickAmountTicked_3`, `m_casterGuid_3`, "
            "`m_numTicks_3`, `m_numTicksCounter_3`, `m_tickTimer_3`, `m_tickIntervalMs_3`) "
            "VALUES ('%u', '%u', '%u', '%d', '%d', '%d', '%d', "
            "'%f', '%f', '%f', '%u', '%d', '%d', '%d', '%d', "
            "'%f', '%f', '%f', '%u', '%d', '%d', '%d', '%d', "
            "'%f', '%f', '%f', '%u', '%d', '%d', '%d', '%d')",
            m_guid, aura.casterGuid, aura.spellId, aura.remainingMs,
            aura.stackCount, aura.spellLevel, aura.casterLevel,
            aura.effects[0].tickTotal, aura.effects[0].tickAmount,
            aura.effects[0].tickAmountTicked, aura.effects[0].casterGuid,
            aura.effects[0].numTicks, aura.effects[0].numTicksCounter,
            aura.effects[0].tickTimer, aura.effects[0].tickIntervalMs,
            aura.effects[1].tickTotal, aura.effects[1].tickAmount,
            aura.effects[1].tickAmountTicked, aura.effects[1].casterGuid,
            aura.effects[1].numTicks, aura.effects[1].numTicksCounter,
            aura.effects[1].tickTimer, aura.effects[1].tickIntervalMs,
            aura.effects[2].tickTotal, aura.effects[2].tickAmount,
            aura.effects[2].tickAmountTicked, aura.effects[2].casterGuid,
            aura.effects[2].numTicks, aura.effects[2].numTicksCounter,
            aura.effects[2].tickTimer, aura.effects[2].tickIntervalMs);
        db.execute(buf);
    }

    // -- Mail --
    snprintf(buf, sizeof(buf), "DELETE FROM `player_mail` WHERE `guid` = '%u'", m_guid);
    db.execute(buf);
    for (const auto& mail : m_mail) {
        if (mail.isEmpty()) continue;
        snprintf(buf, sizeof(buf),
            "INSERT INTO `player_mail` (`guid`, `entry`, `affix`, `affix_score`, `count`, "
            "`gem1`, `gem2`, `gem3`, `gem4`, `enchant_level`, `soulbound`, `durability`) "
            "VALUES('%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%d')",
            m_guid, mail.entry, mail.affix, mail.affixScore, mail.count,
            mail.gem1, mail.gem2, mail.gem3, mail.gem4,
            mail.enchantLevel, mail.soulbound ? 1 : 0, mail.durability);
        db.execute(buf);
    }

    LOG_DEBUG("ServerPlayer::saveToDB() completed for %s (guid %u)", m_name.c_str(), m_guid);
}
