#pragma once
#include "game/ServerUnit.h"
#include "game/templates/ItemTemplate.h"
#include "game/templates/QuestTemplate.h"
#include <array>
#include <set>

class Session;
class GamePacket;

// Spell cooldown entry
struct SpellCooldown {
    uint32 spellId   = 0;
    int64  startDate = 0;
    int64  endDate   = 0;
};

// Spell entry in spellbook
struct PlayerSpell {
    uint32 spellId = 0;
    uint32 level   = 0;
};

// Stat investment entry
struct StatInvestment {
    uint32 statType = 0;
    int32  amount   = 0;
};

// Guild info for a player
struct PlayerGuildInfo {
    uint32 guildId = 0;
    uint32 rank    = 0;
};

// Player data matching the original ServerPlayer.cpp
// Fields verified from binary SQL at 0x00587638
class ServerPlayer : public ServerUnit {
public:
    ServerPlayer();
    ~ServerPlayer() override;

    // Session
    Session* getSession() const { return m_session; }
    void setSession(Session* session) { m_session = session; }

    // Account info
    uint32 getAccountId() const { return m_accountId; }
    void setAccountId(uint32 id) { m_accountId = id; }
    bool isGM() const { return m_isGM; }
    bool isAdmin() const { return m_isAdmin; }
    void setGM(bool gm) { m_isGM = gm; }
    void setAdmin(bool admin) { m_isAdmin = admin; }

    // Character data
    uint8  getPortrait() const { return m_portrait; }
    void   setPortrait(uint8 p) { m_portrait = p; }
    uint32 getXp() const { return m_xp; }
    void   setXp(uint32 xp) { m_xp = xp; }
    void   addXp(uint32 amount);
    uint32 getMoney() const { return m_money; }
    void   setMoney(uint32 money) { m_money = money; }
    void   addMoney(int32 amount);

    // Home position
    uint32 getHomeMap() const { return m_homeMap; }
    float  getHomeX() const { return m_homeX; }
    float  getHomeY() const { return m_homeY; }
    void   setHome(uint32 map, float x, float y);

    // PvP
    int32  getPvpPoints() const { return m_pvpPoints; }
    int32  getPkCount() const { return m_pkCount; }
    uint8  getPvpFlag() const { return m_pvpFlag; }
    void   setPvpPoints(int32 pts) { m_pvpPoints = pts; }
    void   setPkCount(int32 count) { m_pkCount = count; }
    void   setPvpFlag(uint8 flag) { m_pvpFlag = flag; }

    // Combat rating
    uint32 getCombatRating() const { return m_combatRating; }
    void   setCombatRating(uint32 rating) { m_combatRating = rating; }

    // Progression
    uint32 getProgression() const { return m_progression; }
    void   setProgression(uint32 prog) { m_progression = prog; }
    uint32 getNumInvestedSpells() const { return m_numInvestedSpells; }
    void   setNumInvestedSpells(uint32 n) { m_numInvestedSpells = n; }

    // Played time
    uint32 getTimePlayed() const { return m_timePlayed; }
    void   setTimePlayed(uint32 secs) { m_timePlayed = secs; }

    // Death time
    int64  getLogoutTime() const { return m_logoutTime; }
    int64  getLastDeathTime() const { return m_lastDeathTime; }
    void   setLogoutTime(int64 t) { m_logoutTime = t; }
    void   setLastDeathTime(int64 t) { m_lastDeathTime = t; }

    // Inventory (0-based slots)
    const ItemInstance& getInventoryItem(uint32 slot) const;
    bool setInventoryItem(uint32 slot, const ItemInstance& item);
    bool addItemToInventory(const ItemInstance& item);
    bool removeInventoryItem(uint32 slot, uint32 count = 1);
    int  findFreeInventorySlot() const;
    bool hasItem(uint32 entry, uint32 count = 1) const;
    uint32 countItem(uint32 entry) const;

    // Equipment
    const ItemInstance& getEquipment(uint32 slot) const;
    bool equipItem(uint32 inventorySlot);
    bool unequipItem(uint32 equipSlot);

    // Bank
    const ItemInstance& getBankItem(uint32 slot) const;
    bool setBankItem(uint32 slot, const ItemInstance& item);
    bool moveToBankSlot(uint32 invSlot, uint32 bankSlot);
    bool moveFromBankSlot(uint32 bankSlot, uint32 invSlot);

    // Spells
    bool learnSpell(uint32 spellId, uint32 level = 1);
    bool unlearnSpell(uint32 spellId);
    bool hasSpell(uint32 spellId) const;
    const PlayerSpell* getSpell(uint32 spellId) const;
    const std::vector<PlayerSpell>& getSpells() const { return m_spells; }

    // Cooldowns
    bool isSpellOnCooldown(uint32 spellId) const;
    void addSpellCooldown(uint32 spellId, int64 startDate, int64 endDate);
    void removeSpellCooldown(uint32 spellId);
    const std::vector<SpellCooldown>& getCooldowns() const { return m_cooldowns; }

    // Quests
    bool acceptQuest(uint32 questEntry);
    bool completeQuest(uint32 questEntry, uint32 rewardChoice = 0);
    bool hasCompletedQuest(uint32 questEntry) const;
    bool hasActiveQuest(uint32 questEntry) const;
    QuestStatus* getQuestStatus(uint32 questEntry);
    void updateQuestObjective(uint32 questEntry, uint32 objectiveIdx, uint32 count);
    const std::vector<QuestStatus>& getQuests() const { return m_quests; }

    // Stat investments
    void investStat(uint32 statType, int32 amount);
    int32 getStatInvestment(uint32 statType) const;
    const std::vector<StatInvestment>& getStatInvestments() const { return m_statInvestments; }

    // Waypoints
    bool hasDiscoveredWaypoint(uint32 objectGuid) const;
    void discoverWaypoint(uint32 objectGuid);
    const std::set<uint32>& getDiscoveredWaypoints() const { return m_discoveredWaypoints; }

    // Guild
    PlayerGuildInfo& getGuildInfo() { return m_guildInfo; }
    const PlayerGuildInfo& getGuildInfo() const { return m_guildInfo; }
    bool isInGuild() const { return m_guildInfo.guildId != 0; }

    // Mail
    const std::vector<ItemInstance>& getMail() const { return m_mail; }
    void addMail(const ItemInstance& item);

    // Ignore list
    bool isIgnoring(uint32 targetAccountId) const;
    void addIgnore(uint32 targetAccountId);
    void removeIgnore(uint32 targetAccountId);

    // Packet sending
    void sendPacket(const GamePacket& packet);

    // Serialization
    void recalculateStats();

    // Update
    void update(int32 deltaMs) override;
    void onDeath(ServerUnit* killer) override;

    // Save to database
    void saveToDB();

private:
    Session* m_session     = nullptr;
    uint32 m_accountId     = 0;
    bool   m_isGM          = false;
    bool   m_isAdmin       = false;
    uint8  m_portrait      = 0;
    uint32 m_xp            = 0;
    uint32 m_money         = 0;
    uint32 m_homeMap       = 0;
    float  m_homeX         = 0.0f;
    float  m_homeY         = 0.0f;
    int64  m_logoutTime    = 0;
    int64  m_lastDeathTime = 0;
    int32  m_pvpPoints     = 0;
    int32  m_pkCount       = 0;
    uint8  m_pvpFlag       = 0;
    uint32 m_progression   = 0;
    uint32 m_numInvestedSpells = 0;
    uint32 m_combatRating  = 0;
    uint32 m_timePlayed    = 0;

    // Inventory / Equipment / Bank
    std::array<ItemInstance, MAX_INVENTORY_SLOTS> m_inventory = {};
    std::array<ItemInstance, MAX_EQUIPMENT_SLOTS> m_equipment = {};
    std::array<ItemInstance, MAX_BANK_SLOTS>      m_bank      = {};

    // Spells and cooldowns
    std::vector<PlayerSpell>   m_spells;
    std::vector<SpellCooldown> m_cooldowns;

    // Quests
    std::vector<QuestStatus>   m_quests;

    // Stat investments
    std::vector<StatInvestment> m_statInvestments;

    // Waypoints
    std::set<uint32> m_discoveredWaypoints;

    // Guild
    PlayerGuildInfo m_guildInfo;

    // Mail
    std::vector<ItemInstance> m_mail;

    // Ignore
    std::set<uint32> m_ignoreList;

    static const ItemInstance s_emptyItem;
};
