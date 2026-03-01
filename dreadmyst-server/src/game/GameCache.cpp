#include "game/GameCache.h"
#include "database/SQLiteConnector.h"
#include "core/Log.h"

// Static empty vectors for safe return references
const std::vector<NpcWaypoint>  GameCache::s_emptyWaypoints;
const std::vector<VendorItem>   GameCache::s_emptyVendorItems;
const std::vector<VendorRandom> GameCache::s_emptyVendorRandom;
const std::vector<LootEntry>    GameCache::s_emptyLootEntries;
const std::vector<GossipEntry>  GameCache::s_emptyGossipEntries;
const std::vector<GossipOption> GameCache::s_emptyGossipOptions;
const std::vector<ScriptEntry>  GameCache::s_emptyScriptEntries;
const std::vector<CreateItem>   GameCache::s_emptyCreateItems;
const std::vector<CreateSpell>  GameCache::s_emptyCreateSpells;

// ---------------------------------------------------------------------------
// loadAll
// ---------------------------------------------------------------------------
bool GameCache::loadAll(SQLiteConnector& db) {
    LOG_INFO("Loading game cache from database...");

    if (!loadItemTemplates(db))       return false;
    if (!loadSpellTemplates(db))      return false;
    if (!loadNpcTemplates(db))        return false;
    if (!loadNpcSpawns(db))           return false;
    if (!loadNpcWaypoints(db))        return false;
    if (!loadVendorItems(db))         return false;
    if (!loadVendorRandom(db))        return false;
    if (!loadQuestTemplates(db))      return false;
    if (!loadMapTemplates(db))        return false;
    if (!loadGameObjTemplates(db))    return false;
    if (!loadGameObjSpawns(db))       return false;
    if (!loadGossip(db))              return false;
    if (!loadGossipOptions(db))       return false;
    if (!loadLoot(db))                return false;
    if (!loadScripts(db))             return false;
    if (!loadDialogs(db))             return false;
    if (!loadAffixTemplates(db))      return false;
    if (!loadGemTemplates(db))        return false;
    if (!loadClassStats(db))          return false;
    if (!loadExpLevels(db))           return false;
    if (!loadCreateItems(db))         return false;
    if (!loadCreateSpells(db))        return false;
    if (!loadCreateKnownWaypoints(db)) return false;
    if (!loadCombineRecipes(db))      return false;
    if (!loadGraveyards(db))          return false;
    if (!loadArenaTemplates(db))      return false;
    if (!loadDungeonTemplates(db))    return false;
    if (!loadZoneTemplates(db))       return false;
    if (!loadMaterialChances(db))     return false;
    if (!loadItemOrbs(db))            return false;
    if (!loadItemPotionsWorldLoot(db)) return false;
    if (!loadNpcModelJunkLoot(db))    return false;
    if (!loadDesirableStats(db))      return false;
    if (!loadDesirableArmor(db))      return false;

    // Load world texts (no separate loader declared in header)
    {
        auto result = db.query("SELECT id, string FROM world_texts");
        if (!result) return false;
        while (result->next()) {
            WorldText wt;
            wt.id   = result->getUint32(0);
            wt.text = result->getString(1);
            m_worldTexts.push_back(std::move(wt));
        }
        LOG_INFO("Loaded %zu world texts", m_worldTexts.size());
    }

    LOG_INFO("Game cache loaded: %zu items, %zu spells, %zu npcs, %zu quests, "
             "%zu maps, %zu game objects, %zu npc spawns, %zu go spawns",
             m_itemTemplates.size(), m_spellTemplates.size(),
             m_npcTemplates.size(), m_questTemplates.size(),
             m_mapTemplates.size(), m_gameObjTemplates.size(),
             m_npcSpawns.size(), m_gameObjSpawns.size());

    return true;
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
const ItemTemplate* GameCache::getItemTemplate(uint32 entry) const {
    auto it = m_itemTemplates.find(entry);
    return it != m_itemTemplates.end() ? &it->second : nullptr;
}

const SpellTemplate* GameCache::getSpellTemplate(uint32 entry) const {
    auto it = m_spellTemplates.find(entry);
    return it != m_spellTemplates.end() ? &it->second : nullptr;
}

const NpcTemplate* GameCache::getNpcTemplate(uint32 entry) const {
    auto it = m_npcTemplates.find(entry);
    return it != m_npcTemplates.end() ? &it->second : nullptr;
}

const QuestTemplate* GameCache::getQuestTemplate(uint32 entry) const {
    auto it = m_questTemplates.find(entry);
    return it != m_questTemplates.end() ? &it->second : nullptr;
}

const MapTemplate* GameCache::getMapTemplate(uint32 id) const {
    auto it = m_mapTemplates.find(id);
    return it != m_mapTemplates.end() ? &it->second : nullptr;
}

const MapTemplate* GameCache::getMapByName(const std::string& name) const {
    auto it = m_mapNameToId.find(name);
    if (it == m_mapNameToId.end())
        return nullptr;
    return getMapTemplate(it->second);
}

const GameObjectTemplate* GameCache::getGameObjTemplate(uint32 entry) const {
    auto it = m_gameObjTemplates.find(entry);
    return it != m_gameObjTemplates.end() ? &it->second : nullptr;
}

const AffixTemplate* GameCache::getAffixTemplate(uint32 entry) const {
    auto it = m_affixTemplates.find(entry);
    return it != m_affixTemplates.end() ? &it->second : nullptr;
}

const GemTemplate* GameCache::getGemTemplate(uint32 gemId) const {
    auto it = m_gemTemplates.find(gemId);
    return it != m_gemTemplates.end() ? &it->second : nullptr;
}

const ClassStats* GameCache::getClassStats(uint32 classId, uint32 level) const {
    uint64 key = (static_cast<uint64>(classId) << 32) | level;
    auto it = m_classStats.find(key);
    return it != m_classStats.end() ? &it->second : nullptr;
}

const ExpLevel* GameCache::getExpLevel(uint32 level) const {
    auto it = m_expLevels.find(level);
    return it != m_expLevels.end() ? &it->second : nullptr;
}

const std::vector<NpcWaypoint>& GameCache::getNpcWaypoints(uint32 pathId) const {
    auto it = m_npcWaypoints.find(pathId);
    return it != m_npcWaypoints.end() ? it->second : s_emptyWaypoints;
}

const std::vector<VendorItem>& GameCache::getVendorItems(uint32 npcEntry) const {
    auto it = m_vendorItems.find(npcEntry);
    return it != m_vendorItems.end() ? it->second : s_emptyVendorItems;
}

const std::vector<VendorRandom>& GameCache::getVendorRandom(uint32 npcEntry) const {
    auto it = m_vendorRandom.find(npcEntry);
    return it != m_vendorRandom.end() ? it->second : s_emptyVendorRandom;
}

const std::vector<LootEntry>& GameCache::getLootEntries(uint32 lootId) const {
    auto it = m_lootEntries.find(lootId);
    return it != m_lootEntries.end() ? it->second : s_emptyLootEntries;
}

const std::vector<GossipEntry>& GameCache::getGossipEntries(uint32 gossipId) const {
    auto it = m_gossipEntries.find(gossipId);
    return it != m_gossipEntries.end() ? it->second : s_emptyGossipEntries;
}

const std::vector<GossipOption>& GameCache::getGossipOptions(uint32 gossipId) const {
    auto it = m_gossipOptions.find(gossipId);
    return it != m_gossipOptions.end() ? it->second : s_emptyGossipOptions;
}

const std::vector<ScriptEntry>& GameCache::getScriptEntries(uint32 scriptId) const {
    auto it = m_scriptEntries.find(scriptId);
    return it != m_scriptEntries.end() ? it->second : s_emptyScriptEntries;
}

const std::vector<CreateItem>& GameCache::getCreateItems(uint32 classId) const {
    auto it = m_createItems.find(classId);
    return it != m_createItems.end() ? it->second : s_emptyCreateItems;
}

const std::vector<CreateSpell>& GameCache::getCreateSpells(uint32 classId) const {
    auto it = m_createSpells.find(classId);
    return it != m_createSpells.end() ? it->second : s_emptyCreateSpells;
}

const Graveyard* GameCache::getGraveyard(uint32 mapId) const {
    auto it = m_graveyards.find(mapId);
    return it != m_graveyards.end() ? &it->second : nullptr;
}

const ArenaTemplate* GameCache::getArenaTemplate() const {
    return m_arenaTemplates.empty() ? nullptr : &m_arenaTemplates[0];
}

const DungeonTemplate* GameCache::getDungeonTemplate(uint32 mapId) const {
    auto it = m_dungeonTemplates.find(mapId);
    return it != m_dungeonTemplates.end() ? &it->second : nullptr;
}

// ---------------------------------------------------------------------------
// Loaders
// ---------------------------------------------------------------------------

bool GameCache::loadItemTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, sort_entry, required_level, weapon_type, armor_type, equip_type, "
        "quality, item_level, durability, sell_price, stack_count, weapon_material, "
        "num_sockets, required_class, quest_offer, flags, generated, buy_cost_ratio, "
        "spell_1, spell_2, spell_3, spell_4, spell_5, "
        "stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3, "
        "stat_type4, stat_value4, stat_type5, stat_value5, stat_type6, stat_value6, "
        "stat_type7, stat_value7, stat_type8, stat_value8, stat_type9, stat_value9, "
        "stat_type10, stat_value10 FROM item_template");
    if (!result) return false;

    while (result->next()) {
        ItemTemplate tmpl;
        int col = 0;
        tmpl.entry          = result->getUint32(col++);
        tmpl.sortEntry      = result->getUint32(col++);
        tmpl.requiredLevel  = result->getUint32(col++);
        tmpl.weaponType     = result->getUint32(col++);
        tmpl.armorType      = result->getUint32(col++);
        tmpl.equipType      = result->getUint32(col++);
        tmpl.quality        = result->getUint32(col++);
        tmpl.itemLevel      = result->getUint32(col++);
        tmpl.durability     = result->getUint32(col++);
        tmpl.sellPrice      = result->getUint32(col++);
        tmpl.stackCount     = result->getUint32(col++);
        tmpl.weaponMaterial = result->getUint32(col++);
        tmpl.numSockets     = result->getUint32(col++);
        tmpl.requiredClass  = result->getUint32(col++);
        tmpl.questOffer     = result->getUint32(col++);
        tmpl.flags          = result->getUint32(col++);
        tmpl.generated      = result->getUint32(col++);
        tmpl.buyCostRatio   = result->getFloat(col++);
        for (int i = 0; i < MAX_ITEM_SPELLS; ++i)
            tmpl.spells[i] = result->getUint32(col++);
        for (int i = 0; i < MAX_ITEM_STATS; ++i) {
            tmpl.stats[i].type  = result->getUint32(col++);
            tmpl.stats[i].value = result->getInt32(col++);
        }
        m_itemTemplates[tmpl.entry] = tmpl;
    }
    LOG_INFO("Loaded %zu item templates", m_itemTemplates.size());
    return true;
}

bool GameCache::loadSpellTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT spell_template.entry, spell_template.name, mana_formula, mana_pct, "
        "effect1, effect1_data1, effect1_data2, effect1_data3, effect1_targetType, "
        "effect1_radius, effect1_positive, effect1_scale_formula, "
        "effect2, effect2_data1, effect2_data2, effect2_data3, effect2_targetType, "
        "effect2_radius, effect2_positive, effect2_scale_formula, "
        "effect3, effect3_data1, effect3_data2, effect3_data3, effect3_targetType, "
        "effect3_radius, effect3_positive, effect3_scale_formula, "
        "maxTargets, dispel, attributes, cast_time, cooldown, cooldown_category, "
        "aura_interrupt_flags, duration, duration_formula, speed, cast_interrupt_flags, "
        "prevention_type, cast_school, magic_roll_school, range, range_min, "
        "stack_amount, required_equipment, health_cost, health_pct_cost, "
        "activated_by_in, activated_by_out, interval, req_caster_mechanic, "
        "req_tar_mechanic, req_caster_aura, req_tar_aura, stat_scale_1, stat_scale_2, "
        "can_level_up, spell_visual.unit_go_animation, spell_visual.unit_cast_animation "
        "FROM spell_template LEFT JOIN spell_visual ON spell_template.entry=spell_visual.entry");
    if (!result) return false;

    while (result->next()) {
        SpellTemplate tmpl;
        int col = 0;
        tmpl.entry       = result->getUint32(col++);
        tmpl.name        = result->getString(col++);
        tmpl.manaFormula = result->getUint32(col++);
        tmpl.manaPct     = result->getFloat(col++);

        for (int i = 0; i < MAX_SPELL_EFFECTS; ++i) {
            tmpl.effects[i].type         = result->getUint32(col++);
            tmpl.effects[i].data1        = result->getInt32(col++);
            tmpl.effects[i].data2        = result->getInt32(col++);
            tmpl.effects[i].data3        = result->getInt32(col++);
            tmpl.effects[i].targetType   = result->getUint32(col++);
            tmpl.effects[i].radius       = result->getFloat(col++);
            tmpl.effects[i].positive     = static_cast<uint8>(result->getUint32(col++));
            tmpl.effects[i].scaleFormula = result->getUint32(col++);
        }

        tmpl.maxTargets         = result->getUint32(col++);
        tmpl.dispel             = result->getUint32(col++);
        tmpl.attributes         = result->getUint32(col++);
        tmpl.castTime           = result->getUint32(col++);
        tmpl.cooldown           = result->getUint32(col++);
        tmpl.cooldownCategory   = result->getUint32(col++);
        tmpl.auraInterruptFlags = result->getUint32(col++);
        tmpl.duration           = result->getUint32(col++);
        tmpl.durationFormula    = result->getUint32(col++);
        tmpl.speed              = result->getFloat(col++);
        tmpl.castInterruptFlags = result->getUint32(col++);
        tmpl.preventionType     = result->getUint32(col++);
        tmpl.castSchool         = result->getUint32(col++);
        tmpl.magicRollSchool    = result->getUint32(col++);
        tmpl.range              = result->getFloat(col++);
        tmpl.rangeMin           = result->getFloat(col++);
        tmpl.stackAmount        = result->getUint32(col++);
        tmpl.requiredEquipment  = result->getUint32(col++);
        tmpl.healthCost         = result->getUint32(col++);
        tmpl.healthPctCost      = result->getFloat(col++);
        tmpl.activatedByIn      = result->getUint32(col++);
        tmpl.activatedByOut     = result->getUint32(col++);
        tmpl.interval           = result->getUint32(col++);
        tmpl.reqCasterMechanic  = result->getUint32(col++);
        tmpl.reqTarMechanic     = result->getUint32(col++);
        tmpl.reqCasterAura      = result->getUint32(col++);
        tmpl.reqTarAura         = result->getUint32(col++);
        tmpl.statScale1         = result->getUint32(col++);
        tmpl.statScale2         = result->getUint32(col++);
        tmpl.canLevelUp         = result->getUint32(col++);
        tmpl.unitGoAnimation    = result->getUint32(col++);
        tmpl.unitCastAnimation  = result->getUint32(col++);

        m_spellTemplates[tmpl.entry] = std::move(tmpl);
    }
    LOG_INFO("Loaded %zu spell templates", m_spellTemplates.size());
    return true;
}

bool GameCache::loadNpcTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, name, model_id, model_scale, min_level, max_level, faction, type, "
        "npc_flags, game_flags, gossip_menu_id, movement_type, path_id, ai_type, "
        "mechanic_immune_mask, resistance_holy, resistance_frost, resistance_shadow, "
        "resistance_fire, strength, agility, intellect, willpower, courage, armor, "
        "health, mana, weapon_value, melee_skill, ranged_skill, ranged_weapon_value, "
        "melee_speed, ranged_speed, loot_green_chance, loot_blue_chance, loot_gold_chance, "
        "loot_purple_chance, custom_loot, custom_gold_ratio, shield_skill, bool_elite, "
        "bool_boss, leash_range, spell_primary, "
        "spell_1_id, spell_1_chance, spell_1_interval, spell_1_cooldown, spell_1_targetType, "
        "spell_2_id, spell_2_chance, spell_2_interval, spell_2_cooldown, spell_2_targetType, "
        "spell_3_id, spell_3_chance, spell_3_interval, spell_3_cooldown, spell_3_targetType, "
        "spell_4_id, spell_4_chance, spell_4_interval, spell_4_cooldown, spell_4_targetType "
        "FROM npc_template");
    if (!result) return false;

    while (result->next()) {
        NpcTemplate tmpl;
        int col = 0;
        tmpl.entry              = result->getUint32(col++);
        tmpl.name               = result->getString(col++);
        tmpl.modelId            = result->getUint32(col++);
        tmpl.modelScale         = result->getFloat(col++);
        tmpl.minLevel           = result->getUint32(col++);
        tmpl.maxLevel           = result->getUint32(col++);
        tmpl.faction            = result->getUint32(col++);
        tmpl.type               = result->getUint32(col++);
        tmpl.npcFlags           = result->getUint32(col++);
        tmpl.gameFlags          = result->getUint32(col++);
        tmpl.gossipMenuId       = result->getUint32(col++);
        tmpl.movementType       = result->getUint32(col++);
        tmpl.pathId             = result->getUint32(col++);
        tmpl.aiType             = result->getUint32(col++);
        tmpl.mechanicImmuneMask = result->getUint32(col++);
        tmpl.resistanceHoly     = result->getUint32(col++);
        tmpl.resistanceFrost    = result->getUint32(col++);
        tmpl.resistanceShadow   = result->getUint32(col++);
        tmpl.resistanceFire     = result->getUint32(col++);
        tmpl.strength           = result->getUint32(col++);
        tmpl.agility            = result->getUint32(col++);
        tmpl.intellect          = result->getUint32(col++);
        tmpl.willpower          = result->getUint32(col++);
        tmpl.courage            = result->getUint32(col++);
        tmpl.armor              = result->getUint32(col++);
        tmpl.health             = result->getUint32(col++);
        tmpl.mana               = result->getUint32(col++);
        tmpl.weaponValue        = result->getUint32(col++);
        tmpl.meleeSkill         = result->getUint32(col++);
        tmpl.rangedSkill        = result->getUint32(col++);
        tmpl.rangedWeaponValue  = result->getUint32(col++);
        tmpl.meleeSpeed         = result->getUint32(col++);
        tmpl.rangedSpeed        = result->getUint32(col++);
        tmpl.lootGreenChance    = result->getFloat(col++);
        tmpl.lootBlueChance     = result->getFloat(col++);
        tmpl.lootGoldChance     = result->getFloat(col++);
        tmpl.lootPurpleChance   = result->getFloat(col++);
        tmpl.customLoot         = result->getUint32(col++);
        tmpl.customGoldRatio    = result->getFloat(col++);
        tmpl.shieldSkill        = result->getUint32(col++);
        tmpl.boolElite          = static_cast<uint8>(result->getUint32(col++));
        tmpl.boolBoss           = static_cast<uint8>(result->getUint32(col++));
        tmpl.leashRange         = result->getFloat(col++);
        tmpl.spellPrimary       = result->getUint32(col++);

        for (int i = 0; i < MAX_NPC_SPELLS; ++i) {
            tmpl.spells[i].spellId    = result->getUint32(col++);
            tmpl.spells[i].chance     = result->getUint32(col++);
            tmpl.spells[i].interval   = result->getUint32(col++);
            tmpl.spells[i].cooldown   = result->getUint32(col++);
            tmpl.spells[i].targetType = result->getUint32(col++);
        }

        m_npcTemplates[tmpl.entry] = std::move(tmpl);
    }
    LOG_INFO("Loaded %zu npc templates", m_npcTemplates.size());
    return true;
}

bool GameCache::loadNpcSpawns(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT guid, entry, map, position_x, position_y, orientation, path_id, "
        "respawn_time, movement_type, wander_distance, death_state, call_for_help "
        "FROM npc WHERE entry IN (SELECT entry FROM npc_template)");
    if (!result) return false;

    while (result->next()) {
        NpcSpawn spawn;
        int col = 0;
        spawn.guid           = result->getUint32(col++);
        spawn.entry          = result->getUint32(col++);
        spawn.map            = result->getUint32(col++);
        spawn.positionX      = result->getFloat(col++);
        spawn.positionY      = result->getFloat(col++);
        spawn.orientation    = result->getFloat(col++);
        spawn.pathId         = result->getUint32(col++);
        spawn.respawnTime    = result->getUint32(col++);
        spawn.movementType   = result->getUint32(col++);
        spawn.wanderDistance  = result->getFloat(col++);
        spawn.deathState     = static_cast<uint8>(result->getUint32(col++));
        spawn.callForHelp    = static_cast<uint8>(result->getUint32(col++));
        m_npcSpawns.push_back(spawn);
    }
    LOG_INFO("Loaded %zu npc spawns", m_npcSpawns.size());
    return true;
}

bool GameCache::loadNpcWaypoints(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT id, point, position_x, position_y, orientation, run, wait_time, script "
        "FROM npc_waypoints");
    if (!result) return false;

    while (result->next()) {
        NpcWaypoint wp;
        int col = 0;
        wp.id          = result->getUint32(col++);
        wp.point       = result->getUint32(col++);
        wp.positionX   = result->getFloat(col++);
        wp.positionY   = result->getFloat(col++);
        wp.orientation = result->getFloat(col++);
        wp.run         = static_cast<uint8>(result->getUint32(col++));
        wp.waitTime    = result->getUint32(col++);
        wp.script      = result->getUint32(col++);
        m_npcWaypoints[wp.id].push_back(wp);
    }

    size_t total = 0;
    for (auto& [pathId, wps] : m_npcWaypoints)
        total += wps.size();
    LOG_INFO("Loaded %zu npc waypoints across %zu paths", total, m_npcWaypoints.size());
    return true;
}

bool GameCache::loadVendorItems(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT npc_entry, item, max_count, restock_cooldown FROM npc_vendor");
    if (!result) return false;

    while (result->next()) {
        VendorItem vi;
        int col = 0;
        vi.npcEntry        = result->getUint32(col++);
        vi.item            = result->getUint32(col++);
        vi.maxCount        = result->getUint32(col++);
        vi.restockCooldown = result->getUint32(col++);
        m_vendorItems[vi.npcEntry].push_back(vi);
    }

    size_t total = 0;
    for (auto& [npc, items] : m_vendorItems)
        total += items.size();
    LOG_INFO("Loaded %zu vendor items for %zu npcs", total, m_vendorItems.size());
    return true;
}

bool GameCache::loadVendorRandom(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT npc_entry, min_level, max_level, equip_type, weapon_type, armor_preference, "
        "green_chance, blue_chance, gold_chance, purple_chance FROM npc_vendor_random");
    if (!result) return false;

    while (result->next()) {
        VendorRandom vr;
        int col = 0;
        vr.npcEntry        = result->getUint32(col++);
        vr.minLevel        = result->getUint32(col++);
        vr.maxLevel        = result->getUint32(col++);
        vr.equipType       = result->getUint32(col++);
        vr.weaponType      = result->getUint32(col++);
        vr.armorPreference = result->getUint32(col++);
        vr.greenChance     = result->getFloat(col++);
        vr.blueChance      = result->getFloat(col++);
        vr.goldChance      = result->getFloat(col++);
        vr.purpleChance    = result->getFloat(col++);
        m_vendorRandom[vr.npcEntry].push_back(vr);
    }

    size_t total = 0;
    for (auto& [npc, cfgs] : m_vendorRandom)
        total += cfgs.size();
    LOG_INFO("Loaded %zu vendor random configs for %zu npcs", total, m_vendorRandom.size());
    return true;
}

bool GameCache::loadQuestTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, min_level, flags, prev_quest1, prev_quest2, prev_quest3, "
        "provided_item, "
        "req_item1, req_item2, req_item3, req_item4, "
        "req_npc1, req_npc2, req_npc3, req_npc4, "
        "req_go1, req_go2, req_go3, req_go4, "
        "req_spell1, req_spell2, req_spell3, req_spell4, "
        "req_count1, req_count2, req_count3, req_count4, "
        "rew_choice1_item, rew_choice1_count, "
        "rew_choice2_item, rew_choice2_count, "
        "rew_choice3_item, rew_choice3_count, "
        "rew_choice4_item, rew_choice4_count, "
        "rew_item1, rew_item1_count, "
        "rew_item2, rew_item2_count, "
        "rew_item3, rew_item3_count, "
        "rew_item4, rew_item4_count, "
        "rew_money, rew_xp, start_script, complete_script, "
        "start_npc_entry, finish_npc_entry "
        "FROM quest_template ORDER BY entry");
    if (!result) return false;

    while (result->next()) {
        QuestTemplate tmpl;
        int col = 0;
        tmpl.entry    = result->getUint32(col++);
        tmpl.minLevel = result->getUint32(col++);
        tmpl.flags    = result->getUint32(col++);

        for (int i = 0; i < MAX_QUEST_PREREQS; ++i)
            tmpl.prevQuest[i] = result->getUint32(col++);

        tmpl.providedItem = result->getUint32(col++);

        for (int i = 0; i < MAX_QUEST_OBJECTIVES; ++i)
            tmpl.reqItem[i] = result->getUint32(col++);
        for (int i = 0; i < MAX_QUEST_OBJECTIVES; ++i)
            tmpl.reqNpc[i] = result->getUint32(col++);
        for (int i = 0; i < MAX_QUEST_OBJECTIVES; ++i)
            tmpl.reqGo[i] = result->getUint32(col++);
        for (int i = 0; i < MAX_QUEST_OBJECTIVES; ++i)
            tmpl.reqSpell[i] = result->getUint32(col++);
        for (int i = 0; i < MAX_QUEST_OBJECTIVES; ++i)
            tmpl.reqCount[i] = result->getUint32(col++);

        for (int i = 0; i < MAX_QUEST_REWARD_CHOICES; ++i) {
            tmpl.rewChoiceItem[i]  = result->getUint32(col++);
            tmpl.rewChoiceCount[i] = result->getUint32(col++);
        }

        for (int i = 0; i < MAX_QUEST_REWARDS; ++i) {
            tmpl.rewItem[i]      = result->getUint32(col++);
            tmpl.rewItemCount[i] = result->getUint32(col++);
        }

        tmpl.rewMoney       = result->getUint32(col++);
        tmpl.rewXp          = result->getUint32(col++);
        tmpl.startScript    = result->getUint32(col++);
        tmpl.completeScript = result->getUint32(col++);
        tmpl.startNpcEntry  = result->getUint32(col++);
        tmpl.finishNpcEntry = result->getUint32(col++);

        m_questTemplates[tmpl.entry] = tmpl;
    }
    LOG_INFO("Loaded %zu quest templates", m_questTemplates.size());
    return true;
}

bool GameCache::loadMapTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT id, name, start_x, start_y, start_o, los_vision FROM map");
    if (!result) return false;

    while (result->next()) {
        MapTemplate tmpl;
        int col = 0;
        tmpl.id        = result->getUint32(col++);
        tmpl.name      = result->getString(col++);
        tmpl.startX    = result->getFloat(col++);
        tmpl.startY    = result->getFloat(col++);
        tmpl.startO    = result->getFloat(col++);
        tmpl.losVision = static_cast<uint8>(result->getUint32(col++));
        m_mapTemplates[tmpl.id] = std::move(tmpl);
        m_mapNameToId[m_mapTemplates[tmpl.id].name] = tmpl.id;
    }
    LOG_INFO("Loaded %zu map templates", m_mapTemplates.size());
    return true;
}

bool GameCache::loadGameObjTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, flags, required_item, required_quest, type, "
        "data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11 "
        "FROM gameobject_template");
    if (!result) return false;

    while (result->next()) {
        GameObjectTemplate tmpl;
        int col = 0;
        tmpl.entry         = result->getUint32(col++);
        tmpl.flags         = result->getUint32(col++);
        tmpl.requiredItem  = result->getUint32(col++);
        tmpl.requiredQuest = result->getUint32(col++);
        tmpl.type          = result->getUint32(col++);
        for (int i = 0; i < MAX_GAMEOBJ_DATA; ++i)
            tmpl.data[i] = result->getInt32(col++);
        m_gameObjTemplates[tmpl.entry] = tmpl;
    }
    LOG_INFO("Loaded %zu gameobject templates", m_gameObjTemplates.size());
    return true;
}

bool GameCache::loadGameObjSpawns(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT guid, entry, map, position_x, position_y, respawn, state "
        "FROM gameobject WHERE entry IN (SELECT entry FROM gameobject_template)");
    if (!result) return false;

    while (result->next()) {
        GameObjectSpawn spawn;
        int col = 0;
        spawn.guid      = result->getUint32(col++);
        spawn.entry     = result->getUint32(col++);
        spawn.map       = result->getUint32(col++);
        spawn.positionX = result->getFloat(col++);
        spawn.positionY = result->getFloat(col++);
        spawn.respawn   = result->getUint32(col++);
        spawn.state     = result->getUint32(col++);
        m_gameObjSpawns.push_back(spawn);
    }
    LOG_INFO("Loaded %zu gameobject spawns", m_gameObjSpawns.size());
    return true;
}

bool GameCache::loadGossip(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, gossipId, text, "
        "condition1, condition1_value1, condition1_value2, condition1_true, "
        "condition2, condition2_value1, condition2_value2, condition2_true, "
        "condition3, condition3_value1, condition3_value2, condition3_true "
        "FROM gossip ORDER BY entry");
    if (!result) return false;

    while (result->next()) {
        GossipEntry ge;
        int col = 0;
        ge.entry    = result->getUint32(col++);
        ge.gossipId = result->getUint32(col++);
        ge.text     = result->getString(col++);
        for (int i = 0; i < MAX_GOSSIP_CONDITIONS; ++i) {
            ge.conditions[i].type   = result->getUint32(col++);
            ge.conditions[i].value1 = result->getInt32(col++);
            ge.conditions[i].value2 = result->getInt32(col++);
            ge.conditions[i].isTrue = static_cast<uint8>(result->getUint32(col++));
        }
        m_gossipEntries[ge.gossipId].push_back(std::move(ge));
    }

    size_t total = 0;
    for (auto& [id, entries] : m_gossipEntries)
        total += entries.size();
    LOG_INFO("Loaded %zu gossip entries across %zu menus", total, m_gossipEntries.size());
    return true;
}

bool GameCache::loadGossipOptions(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, gossipId, text, required_npc_flag, click_new_gossip, click_script, "
        "condition1, condition1_value1, condition1_value2, condition1_true, "
        "condition2, condition2_value1, condition2_value2, condition2_true, "
        "condition3, condition3_value1, condition3_value2, condition3_true "
        "FROM gossip_option ORDER BY entry");
    if (!result) return false;

    while (result->next()) {
        GossipOption go;
        int col = 0;
        go.entry           = result->getUint32(col++);
        go.gossipId        = result->getUint32(col++);
        go.text            = result->getString(col++);
        go.requiredNpcFlag = result->getUint32(col++);
        go.clickNewGossip  = result->getUint32(col++);
        go.clickScript     = result->getUint32(col++);
        for (int i = 0; i < MAX_GOSSIP_CONDITIONS; ++i) {
            go.conditions[i].type   = result->getUint32(col++);
            go.conditions[i].value1 = result->getInt32(col++);
            go.conditions[i].value2 = result->getInt32(col++);
            go.conditions[i].isTrue = static_cast<uint8>(result->getUint32(col++));
        }
        m_gossipOptions[go.gossipId].push_back(std::move(go));
    }

    size_t total = 0;
    for (auto& [id, opts] : m_gossipOptions)
        total += opts.size();
    LOG_INFO("Loaded %zu gossip options across %zu menus", total, m_gossipOptions.size());
    return true;
}

bool GameCache::loadLoot(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, lootId, item, chance, count_min, count_max, "
        "condition1, condition1_value1, condition1_value2, condition1_true, "
        "condition2, condition2_value1, condition2_value2, condition2_true "
        "FROM loot");
    if (!result) return false;

    while (result->next()) {
        LootEntry le;
        int col = 0;
        le.entry    = result->getUint32(col++);
        le.lootId   = result->getUint32(col++);
        le.item     = result->getUint32(col++);
        le.chance   = result->getFloat(col++);
        le.countMin = result->getUint32(col++);
        le.countMax = result->getUint32(col++);
        for (int i = 0; i < MAX_LOOT_CONDITIONS; ++i) {
            le.conditions[i].type   = result->getUint32(col++);
            le.conditions[i].value1 = result->getInt32(col++);
            le.conditions[i].value2 = result->getInt32(col++);
            le.conditions[i].isTrue = static_cast<uint8>(result->getUint32(col++));
        }
        m_lootEntries[le.lootId].push_back(le);
    }

    size_t total = 0;
    for (auto& [id, entries] : m_lootEntries)
        total += entries.size();
    LOG_INFO("Loaded %zu loot entries across %zu loot tables", total, m_lootEntries.size());
    return true;
}

bool GameCache::loadScripts(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, scriptId, delay, command, data1, data2, data3, data4, data5 "
        "FROM scripts");
    if (!result) return false;

    while (result->next()) {
        ScriptEntry se;
        int col = 0;
        se.entry    = result->getUint32(col++);
        se.scriptId = result->getUint32(col++);
        se.delay    = result->getUint32(col++);
        se.command  = result->getUint32(col++);
        se.data1    = result->getInt32(col++);
        se.data2    = result->getInt32(col++);
        se.data3    = result->getInt32(col++);
        se.data4    = result->getInt32(col++);
        se.data5    = result->getInt32(col++);
        m_scriptEntries[se.scriptId].push_back(se);
    }

    size_t total = 0;
    for (auto& [id, entries] : m_scriptEntries)
        total += entries.size();
    LOG_INFO("Loaded %zu script entries across %zu scripts", total, m_scriptEntries.size());
    return true;
}

bool GameCache::loadDialogs(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, text, sound, type FROM dialog");
    if (!result) return false;

    while (result->next()) {
        DialogEntry de;
        int col = 0;
        de.entry = result->getUint32(col++);
        de.text  = result->getString(col++);
        de.sound = result->getUint32(col++);
        de.type  = result->getUint32(col++);
        m_dialogs.push_back(std::move(de));
    }
    LOG_INFO("Loaded %zu dialog entries", m_dialogs.size());
    return true;
}

bool GameCache::loadAffixTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, min_level, max_level, "
        "stat_type1, stat_value1, stat_type2, stat_value2, stat_type3, stat_value3, "
        "stat_type4, stat_value4, stat_type5, stat_value5 "
        "FROM affix_template");
    if (!result) return false;

    while (result->next()) {
        AffixTemplate tmpl;
        int col = 0;
        tmpl.entry    = result->getUint32(col++);
        tmpl.minLevel = result->getUint32(col++);
        tmpl.maxLevel = result->getUint32(col++);
        for (int i = 0; i < MAX_AFFIX_STATS; ++i) {
            tmpl.stats[i].type  = result->getUint32(col++);
            tmpl.stats[i].value = result->getInt32(col++);
        }
        m_affixTemplates[tmpl.entry] = tmpl;
    }
    LOG_INFO("Loaded %zu affix templates", m_affixTemplates.size());
    return true;
}

bool GameCache::loadGemTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT gem_id, quality, stat_1, stat_1_amount, stat_2, stat_2_amount "
        "FROM item_gems");
    if (!result) return false;

    while (result->next()) {
        GemTemplate tmpl;
        int col = 0;
        tmpl.gemId      = result->getUint32(col++);
        tmpl.quality    = result->getUint32(col++);
        tmpl.stat1      = result->getUint32(col++);
        tmpl.stat1Amount = result->getInt32(col++);
        tmpl.stat2      = result->getUint32(col++);
        tmpl.stat2Amount = result->getInt32(col++);
        m_gemTemplates[tmpl.gemId] = tmpl;
    }
    LOG_INFO("Loaded %zu gem templates", m_gemTemplates.size());
    return true;
}

bool GameCache::loadClassStats(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT class, level, hp, mana, strength, agility, willpower, intelligence, courage "
        "FROM player_class_stats");
    if (!result) return false;

    while (result->next()) {
        ClassStats cs;
        int col = 0;
        cs.classId      = result->getUint32(col++);
        cs.level        = result->getUint32(col++);
        cs.hp           = result->getUint32(col++);
        cs.mana         = result->getUint32(col++);
        cs.strength     = result->getUint32(col++);
        cs.agility      = result->getUint32(col++);
        cs.willpower    = result->getUint32(col++);
        cs.intelligence = result->getUint32(col++);
        cs.courage      = result->getUint32(col++);
        uint64 key = (static_cast<uint64>(cs.classId) << 32) | cs.level;
        m_classStats[key] = cs;
    }
    LOG_INFO("Loaded %zu class stat entries", m_classStats.size());
    return true;
}

bool GameCache::loadExpLevels(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT level, exp, kill_exp FROM player_exp_levels");
    if (!result) return false;

    while (result->next()) {
        ExpLevel el;
        int col = 0;
        el.level   = result->getUint32(col++);
        el.exp     = result->getUint32(col++);
        el.killExp = result->getUint32(col++);
        m_expLevels[el.level] = el;
    }
    LOG_INFO("Loaded %zu exp level entries", m_expLevels.size());
    return true;
}

bool GameCache::loadCreateItems(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT class, item, count FROM player_create_item");
    if (!result) return false;

    while (result->next()) {
        CreateItem ci;
        int col = 0;
        ci.classId = result->getUint32(col++);
        ci.item    = result->getUint32(col++);
        ci.count   = result->getUint32(col++);
        m_createItems[ci.classId].push_back(ci);
    }

    size_t total = 0;
    for (auto& [cls, items] : m_createItems)
        total += items.size();
    LOG_INFO("Loaded %zu player create items for %zu classes", total, m_createItems.size());
    return true;
}

bool GameCache::loadCreateSpells(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT class, spell FROM player_create_spell");
    if (!result) return false;

    while (result->next()) {
        CreateSpell cs;
        int col = 0;
        cs.classId = result->getUint32(col++);
        cs.spell   = result->getUint32(col++);
        m_createSpells[cs.classId].push_back(cs);
    }

    size_t total = 0;
    for (auto& [cls, spells] : m_createSpells)
        total += spells.size();
    LOG_INFO("Loaded %zu player create spells for %zu classes", total, m_createSpells.size());
    return true;
}

bool GameCache::loadCreateKnownWaypoints(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT entry, object_guid FROM player_create_known_waypoints");
    if (!result) return false;

    while (result->next()) {
        CreateKnownWaypoint ckw;
        int col = 0;
        ckw.entry      = result->getUint32(col++);
        ckw.objectGuid = result->getUint32(col++);
        m_createKnownWaypoints.push_back(ckw);
    }
    LOG_INFO("Loaded %zu player create known waypoints", m_createKnownWaypoints.size());
    return true;
}

bool GameCache::loadCombineRecipes(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT item_id, item_id_2, output_item FROM combine_items");
    if (!result) return false;

    while (result->next()) {
        CombineRecipe cr;
        int col = 0;
        cr.itemId1    = result->getUint32(col++);
        cr.itemId2    = result->getUint32(col++);
        cr.outputItem = result->getUint32(col++);
        m_combineRecipes.push_back(cr);
    }
    LOG_INFO("Loaded %zu combine recipes", m_combineRecipes.size());
    return true;
}

bool GameCache::loadGraveyards(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT map, respawn_map, respawn_x, respawn_y FROM graveyard");
    if (!result) return false;

    while (result->next()) {
        Graveyard gy;
        int col = 0;
        gy.map       = result->getUint32(col++);
        gy.respawnMap = result->getUint32(col++);
        gy.respawnX  = result->getFloat(col++);
        gy.respawnY  = result->getFloat(col++);
        m_graveyards[gy.map] = gy;
    }
    LOG_INFO("Loaded %zu graveyards", m_graveyards.size());
    return true;
}

bool GameCache::loadArenaTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT map, "
        "player_1_x, player_1_y, player_2_x, player_2_y, "
        "player_3_x, player_3_y, player_4_x, player_4_y, "
        "player_1_x_start, player_1_y_start, player_2_x_start, player_2_y_start, "
        "player_3_x_start, player_3_y_start, player_4_x_start, player_4_y_start "
        "FROM arena_template");
    if (!result) return false;

    while (result->next()) {
        ArenaTemplate at;
        int col = 0;
        at.map           = result->getUint32(col++);
        at.player1X      = result->getFloat(col++);
        at.player1Y      = result->getFloat(col++);
        at.player2X      = result->getFloat(col++);
        at.player2Y      = result->getFloat(col++);
        at.player3X      = result->getFloat(col++);
        at.player3Y      = result->getFloat(col++);
        at.player4X      = result->getFloat(col++);
        at.player4Y      = result->getFloat(col++);
        at.player1XStart = result->getFloat(col++);
        at.player1YStart = result->getFloat(col++);
        at.player2XStart = result->getFloat(col++);
        at.player2YStart = result->getFloat(col++);
        at.player3XStart = result->getFloat(col++);
        at.player3YStart = result->getFloat(col++);
        at.player4XStart = result->getFloat(col++);
        at.player4YStart = result->getFloat(col++);
        m_arenaTemplates.push_back(at);
    }
    LOG_INFO("Loaded %zu arena templates", m_arenaTemplates.size());
    return true;
}

bool GameCache::loadDungeonTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT map, max_players, required_level, required_item, required_quest "
        "FROM dungeon_template");
    if (!result) return false;

    while (result->next()) {
        DungeonTemplate dt;
        int col = 0;
        dt.map           = result->getUint32(col++);
        dt.maxPlayers    = result->getUint32(col++);
        dt.requiredLevel = result->getUint32(col++);
        dt.requiredItem  = result->getUint32(col++);
        dt.requiredQuest = result->getUint32(col++);
        m_dungeonTemplates[dt.map] = dt;
    }
    LOG_INFO("Loaded %zu dungeon templates", m_dungeonTemplates.size());
    return true;
}

bool GameCache::loadZoneTemplates(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT id, type FROM zone_template");
    if (!result) return false;

    while (result->next()) {
        ZoneTemplate zt;
        int col = 0;
        zt.id   = result->getUint32(col++);
        zt.type = result->getUint32(col++);
        m_zoneTemplates[zt.id] = zt;
    }
    LOG_INFO("Loaded %zu zone templates", m_zoneTemplates.size());
    return true;
}

bool GameCache::loadMaterialChances(SQLiteConnector& db) {
    // Armor material chances
    auto result = db.query(
        "SELECT level, armor_type, chance FROM material_chance_armor");
    if (!result) return false;

    while (result->next()) {
        MaterialChance mc;
        int col = 0;
        mc.level  = result->getUint32(col++);
        mc.type   = result->getUint32(col++);
        mc.chance = result->getFloat(col++);
        m_materialChanceArmor.push_back(mc);
    }
    LOG_INFO("Loaded %zu armor material chances", m_materialChanceArmor.size());

    // Weapon material chances
    result = db.query(
        "SELECT level, weapon_material, chance FROM material_chance_weapon");
    if (!result) return false;

    while (result->next()) {
        MaterialChance mc;
        int col = 0;
        mc.level  = result->getUint32(col++);
        mc.type   = result->getUint32(col++);
        mc.chance = result->getFloat(col++);
        m_materialChanceWeapon.push_back(mc);
    }
    LOG_INFO("Loaded %zu weapon material chances", m_materialChanceWeapon.size());
    return true;
}

bool GameCache::loadItemOrbs(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT item_entry FROM item_orbs");
    if (!result) return false;

    while (result->next()) {
        m_itemOrbs.push_back(result->getUint32(0));
    }
    LOG_INFO("Loaded %zu item orbs", m_itemOrbs.size());
    return true;
}

bool GameCache::loadItemPotionsWorldLoot(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT item_entry FROM item_potions_worldloot");
    if (!result) return false;

    while (result->next()) {
        m_itemPotionsWorldLoot.push_back(result->getUint32(0));
    }
    LOG_INFO("Loaded %zu item potions world loot entries", m_itemPotionsWorldLoot.size());
    return true;
}

bool GameCache::loadNpcModelJunkLoot(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT model_id, item_entry FROM npc_models_junkloot");
    if (!result) return false;

    while (result->next()) {
        NpcModelJunkLoot jl;
        int col = 0;
        jl.modelId   = result->getUint32(col++);
        jl.itemEntry = result->getUint32(col++);
        m_npcModelJunkLoot.push_back(jl);
    }
    LOG_INFO("Loaded %zu npc model junk loot entries", m_npcModelJunkLoot.size());
    return true;
}

bool GameCache::loadDesirableStats(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT class_id, stat_id FROM player_desirable_stats");
    if (!result) return false;

    while (result->next()) {
        DesirableStat ds;
        int col = 0;
        ds.classId = result->getUint32(col++);
        ds.statId  = result->getUint32(col++);
        m_desirableStats.push_back(ds);
    }
    LOG_INFO("Loaded %zu desirable stat entries", m_desirableStats.size());
    return true;
}

bool GameCache::loadDesirableArmor(SQLiteConnector& db) {
    auto result = db.query(
        "SELECT level, class_id, armor_type FROM player_desirable_armor");
    if (!result) return false;

    while (result->next()) {
        DesirableArmor da;
        int col = 0;
        da.level     = result->getUint32(col++);
        da.classId   = result->getUint32(col++);
        da.armorType = result->getUint32(col++);
        m_desirableArmor.push_back(da);
    }
    LOG_INFO("Loaded %zu desirable armor entries", m_desirableArmor.size());
    return true;
}

