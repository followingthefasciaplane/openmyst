#pragma once
#include "core/Types.h"
#include "game/templates/ItemTemplate.h"
#include "game/templates/SpellTemplate.h"
#include "game/templates/NpcTemplate.h"
#include "game/templates/QuestTemplate.h"
#include "game/templates/MapTemplate.h"
#include "game/templates/GameObjectTemplate.h"
#include "game/templates/ClassStats.h"
#include <unordered_map>
#include <vector>
#include <string>

class SQLiteConnector;

// In-memory cache of all game template data loaded from game.db at startup
// Matches the original GameCache.cpp behavior
class GameCache {
public:
    bool loadAll(SQLiteConnector& db);

    // Template lookups
    const ItemTemplate*       getItemTemplate(uint32 entry) const;
    const SpellTemplate*      getSpellTemplate(uint32 entry) const;
    const NpcTemplate*        getNpcTemplate(uint32 entry) const;
    const QuestTemplate*      getQuestTemplate(uint32 entry) const;
    const MapTemplate*        getMapTemplate(uint32 id) const;
    const MapTemplate*        getMapByName(const std::string& name) const;
    const GameObjectTemplate* getGameObjTemplate(uint32 entry) const;
    const AffixTemplate*      getAffixTemplate(uint32 entry) const;
    const GemTemplate*        getGemTemplate(uint32 gemId) const;

    // Class/level stat lookups
    const ClassStats*         getClassStats(uint32 classId, uint32 level) const;
    const ExpLevel*           getExpLevel(uint32 level) const;

    // Collection accessors
    const std::vector<NpcSpawn>&       getNpcSpawns() const { return m_npcSpawns; }
    const std::vector<GameObjectSpawn>& getGameObjSpawns() const { return m_gameObjSpawns; }
    const std::vector<NpcWaypoint>&    getNpcWaypoints(uint32 pathId) const;
    const std::vector<VendorItem>&     getVendorItems(uint32 npcEntry) const;
    const std::vector<VendorRandom>&   getVendorRandom(uint32 npcEntry) const;
    const std::vector<LootEntry>&      getLootEntries(uint32 lootId) const;
    const std::vector<GossipEntry>&    getGossipEntries(uint32 gossipId) const;
    const std::vector<GossipOption>&   getGossipOptions(uint32 gossipId) const;
    const std::vector<ScriptEntry>&    getScriptEntries(uint32 scriptId) const;
    const std::vector<CreateItem>&     getCreateItems(uint32 classId) const;
    const std::vector<CreateSpell>&    getCreateSpells(uint32 classId) const;
    const std::vector<CombineRecipe>&  getCombineRecipes() const { return m_combineRecipes; }
    const Graveyard*                   getGraveyard(uint32 mapId) const;
    const ArenaTemplate*               getArenaTemplate() const;
    const DungeonTemplate*             getDungeonTemplate(uint32 mapId) const;

    // World data
    const std::vector<DialogEntry>&    getDialogs() const { return m_dialogs; }
    const std::vector<WorldText>&      getWorldTexts() const { return m_worldTexts; }

    uint32 getMaxPlayerGuid() const { return m_maxPlayerGuid; }
    void setMaxPlayerGuid(uint32 guid) { m_maxPlayerGuid = guid; }

private:
    // Loaders
    bool loadItemTemplates(SQLiteConnector& db);
    bool loadSpellTemplates(SQLiteConnector& db);
    bool loadNpcTemplates(SQLiteConnector& db);
    bool loadNpcSpawns(SQLiteConnector& db);
    bool loadNpcWaypoints(SQLiteConnector& db);
    bool loadVendorItems(SQLiteConnector& db);
    bool loadVendorRandom(SQLiteConnector& db);
    bool loadQuestTemplates(SQLiteConnector& db);
    bool loadMapTemplates(SQLiteConnector& db);
    bool loadGameObjTemplates(SQLiteConnector& db);
    bool loadGameObjSpawns(SQLiteConnector& db);
    bool loadGossip(SQLiteConnector& db);
    bool loadGossipOptions(SQLiteConnector& db);
    bool loadLoot(SQLiteConnector& db);
    bool loadScripts(SQLiteConnector& db);
    bool loadDialogs(SQLiteConnector& db);
    bool loadAffixTemplates(SQLiteConnector& db);
    bool loadGemTemplates(SQLiteConnector& db);
    bool loadClassStats(SQLiteConnector& db);
    bool loadExpLevels(SQLiteConnector& db);
    bool loadCreateItems(SQLiteConnector& db);
    bool loadCreateSpells(SQLiteConnector& db);
    bool loadCreateKnownWaypoints(SQLiteConnector& db);
    bool loadCombineRecipes(SQLiteConnector& db);
    bool loadGraveyards(SQLiteConnector& db);
    bool loadArenaTemplates(SQLiteConnector& db);
    bool loadDungeonTemplates(SQLiteConnector& db);
    bool loadZoneTemplates(SQLiteConnector& db);
    bool loadMaterialChances(SQLiteConnector& db);
    bool loadItemOrbs(SQLiteConnector& db);
    bool loadItemPotionsWorldLoot(SQLiteConnector& db);
    bool loadNpcModelJunkLoot(SQLiteConnector& db);
    bool loadDesirableStats(SQLiteConnector& db);
    bool loadDesirableArmor(SQLiteConnector& db);

    // Storage
    std::unordered_map<uint32, ItemTemplate>       m_itemTemplates;
    std::unordered_map<uint32, SpellTemplate>      m_spellTemplates;
    std::unordered_map<uint32, NpcTemplate>        m_npcTemplates;
    std::unordered_map<uint32, QuestTemplate>      m_questTemplates;
    std::unordered_map<uint32, MapTemplate>        m_mapTemplates;
    std::unordered_map<std::string, uint32>        m_mapNameToId;
    std::unordered_map<uint32, GameObjectTemplate> m_gameObjTemplates;
    std::unordered_map<uint32, AffixTemplate>      m_affixTemplates;
    std::unordered_map<uint32, GemTemplate>        m_gemTemplates;
    std::unordered_map<uint32, ZoneTemplate>       m_zoneTemplates;

    std::vector<NpcSpawn>       m_npcSpawns;
    std::vector<GameObjectSpawn> m_gameObjSpawns;

    std::unordered_map<uint32, std::vector<NpcWaypoint>>  m_npcWaypoints;   // pathId -> waypoints
    std::unordered_map<uint32, std::vector<VendorItem>>   m_vendorItems;    // npcEntry -> items
    std::unordered_map<uint32, std::vector<VendorRandom>> m_vendorRandom;   // npcEntry -> random configs
    std::unordered_map<uint32, std::vector<LootEntry>>    m_lootEntries;    // lootId -> entries
    std::unordered_map<uint32, std::vector<GossipEntry>>  m_gossipEntries;  // gossipId -> entries
    std::unordered_map<uint32, std::vector<GossipOption>> m_gossipOptions;  // gossipId -> options
    std::unordered_map<uint32, std::vector<ScriptEntry>>  m_scriptEntries;  // scriptId -> entries

    // Class/level data
    std::unordered_map<uint64, ClassStats> m_classStats;  // (classId<<32|level) -> stats
    std::unordered_map<uint32, ExpLevel>   m_expLevels;
    std::unordered_map<uint32, std::vector<CreateItem>>  m_createItems;   // classId -> items
    std::unordered_map<uint32, std::vector<CreateSpell>> m_createSpells;  // classId -> spells
    std::vector<CreateKnownWaypoint> m_createKnownWaypoints;

    std::vector<CombineRecipe>  m_combineRecipes;
    std::unordered_map<uint32, Graveyard> m_graveyards;   // mapId -> graveyard
    std::vector<ArenaTemplate>  m_arenaTemplates;
    std::unordered_map<uint32, DungeonTemplate> m_dungeonTemplates;

    std::vector<DialogEntry>    m_dialogs;
    std::vector<WorldText>      m_worldTexts;

    std::vector<MaterialChance> m_materialChanceArmor;
    std::vector<MaterialChance> m_materialChanceWeapon;
    std::vector<uint32>         m_itemOrbs;
    std::vector<uint32>         m_itemPotionsWorldLoot;
    std::vector<NpcModelJunkLoot> m_npcModelJunkLoot;
    std::vector<DesirableStat>  m_desirableStats;
    std::vector<DesirableArmor> m_desirableArmor;

    uint32 m_maxPlayerGuid = 0;

    static const std::vector<NpcWaypoint>  s_emptyWaypoints;
    static const std::vector<VendorItem>   s_emptyVendorItems;
    static const std::vector<VendorRandom> s_emptyVendorRandom;
    static const std::vector<LootEntry>    s_emptyLootEntries;
    static const std::vector<GossipEntry>  s_emptyGossipEntries;
    static const std::vector<GossipOption> s_emptyGossipOptions;
    static const std::vector<ScriptEntry>  s_emptyScriptEntries;
    static const std::vector<CreateItem>   s_emptyCreateItems;
    static const std::vector<CreateSpell>  s_emptyCreateSpells;
};
