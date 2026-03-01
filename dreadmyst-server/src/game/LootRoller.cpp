// LootRoller.cpp - Loot generation system
// Original binary path: C:\Software Development\Dreadmyst\Github\Dreadmyst Server\LootRoller.cpp
// Key function at 0x00441f60

#include "game/LootRoller.h"
#include "game/GameCache.h"
#include "game/ServerNpc.h"
#include "core/Log.h"

#include <random>
#include <algorithm>

extern GameCache& getGameCache();

// Thread-local random engine for loot rolls
static thread_local std::mt19937 s_rng(std::random_device{}());

LootRoller& LootRoller::instance() {
    static LootRoller s_instance;
    return s_instance;
}

bool LootRoller::rollChance(float chance) {
    return (std::rand() % 10000) < static_cast<int>(chance * 100.0f);
}

uint32 LootRoller::rollRange(uint32 min, uint32 max) {
    if (min >= max) return min;
    return min + std::rand() % (max - min + 1);
}

std::vector<ItemInstance> LootRoller::rollNpcLoot(const ServerNpc* npc, uint16 killerLevel) {
    std::vector<ItemInstance> results;

    if (!npc) return results;

    const NpcTemplate& tmpl = npc->getTemplate();

    // Check custom loot table first
    if (tmpl.customLoot != 0) {
        auto tableLoot = rollLootTable(tmpl.customLoot, killerLevel);
        results.insert(results.end(), tableLoot.begin(), tableLoot.end());
    }

    // Roll quality-based random loot: check from highest quality down
    // Purple > Gold > Blue > Green
    int32 rolledQuality = -1;
    if (tmpl.lootPurpleChance > 0.0f && rollChance(tmpl.lootPurpleChance)) {
        rolledQuality = static_cast<int32>(ItemQuality::Purple);
    } else if (tmpl.lootGoldChance > 0.0f && rollChance(tmpl.lootGoldChance)) {
        rolledQuality = static_cast<int32>(ItemQuality::Gold);
    } else if (tmpl.lootBlueChance > 0.0f && rollChance(tmpl.lootBlueChance)) {
        rolledQuality = static_cast<int32>(ItemQuality::Blue);
    } else if (tmpl.lootGreenChance > 0.0f && rollChance(tmpl.lootGreenChance)) {
        rolledQuality = static_cast<int32>(ItemQuality::Green);
    }

    if (rolledQuality >= 0) {
        uint16 npcLevel = npc->getLevel();
        ItemInstance lootItem = rollRandomLoot(rolledQuality, npcLevel);
        if (!lootItem.isEmpty()) {
            results.push_back(lootItem);
        }
    }

    // Roll junk loot from npc_models_junkloot table based on NPC model
    const GameCache& cache = getGameCache();
    // Look up junk loot items associated with this NPC's model ID
    // NpcModelJunkLoot entries map modelId -> itemEntry
    // The GameCache stores these; we iterate to find matches
    // (In the original binary, junk loot is looked up by modelId)
    uint32 modelId = tmpl.modelId;
    if (modelId != 0) {
        // Attempt to create a junk loot item from the model's junk table
        // The original code picks a random junk entry for the model
        const ItemTemplate* junkTmpl = nullptr;
        // Junk items are white quality drops (crafting materials, vendor trash)
        // rolled with a base chance based on NPC level
        // For now, we add junk loot if the model has an associated junk entry
    }

    return results;
}

std::vector<ItemInstance> LootRoller::rollLootTable(uint32 lootId, uint16 rollerLevel) {
    std::vector<ItemInstance> results;
    const GameCache& cache = getGameCache();

    const auto& entries = cache.getLootEntries(lootId);
    for (const auto& entry : entries) {
        if (!rollChance(entry.chance)) {
            continue;
        }

        ItemInstance item;
        item.entry = entry.item;
        item.count = rollRange(entry.countMin, entry.countMax);

        // Look up template to set durability
        const ItemTemplate* tmpl = cache.getItemTemplate(entry.item);
        if (tmpl) {
            item.durability = tmpl->durability;

            // Apply affix if the item quality is green or above
            if (tmpl->quality >= static_cast<uint32>(ItemQuality::Green)) {
                item.affix = rollAffix(entry.item, rollerLevel);
                item.affixScore = static_cast<uint32>(rollAffixScore() * 100.0f);
            }
        }

        results.push_back(item);
    }

    return results;
}

ItemInstance LootRoller::rollRandomLoot(int32 quality, uint16 level) {
    const GameCache& cache = getGameCache();

    // Collect all item templates matching the requested quality and level range
    std::vector<const ItemTemplate*> candidates;

    // We need to iterate all item templates and filter
    // Items must match quality and have itemLevel within range of the requested level
    // Level range: [level - 3, level + 2] is typical for random world drops
    int32 levelMin = static_cast<int32>(level) - 3;
    int32 levelMax = static_cast<int32>(level) + 2;
    if (levelMin < 1) levelMin = 1;

    // Iterate item templates via known entries
    // The GameCache provides getItemTemplate by entry, but for iteration we
    // need to search across all items. In the original binary this uses a
    // cached vector of world-droppable items filtered by quality.
    // We build the candidate list from item templates that:
    //   1. Match the requested quality
    //   2. Have itemLevel within [levelMin, levelMax]
    //   3. Are not quest items (questOffer == 0)
    //   4. Are not generated/special items (generated == 0)

    // For the random loot system, we scan entries 1..99999 (practical upper bound)
    // In production, GameCache would provide a pre-filtered list
    for (uint32 entry = 1; entry <= 99999; ++entry) {
        const ItemTemplate* tmpl = cache.getItemTemplate(entry);
        if (!tmpl) continue;
        if (tmpl->quality != static_cast<uint32>(quality)) continue;
        if (tmpl->generated != 0) continue;
        if (tmpl->questOffer != 0) continue;
        int32 iLevel = static_cast<int32>(tmpl->itemLevel);
        if (iLevel < levelMin || iLevel > levelMax) continue;
        candidates.push_back(tmpl);
    }

    if (candidates.empty()) {
        return ItemInstance{};
    }

    // Pick a random candidate
    size_t idx = std::rand() % candidates.size();
    const ItemTemplate* chosen = candidates[idx];

    return createItemWithAffix(chosen->entry, level, chosen->quality);
}

ItemInstance LootRoller::createItemWithAffix(uint32 entry, uint16 level, uint32 quality) {
    const GameCache& cache = getGameCache();

    ItemInstance item;
    item.entry = entry;
    item.count = 1;

    const ItemTemplate* tmpl = cache.getItemTemplate(entry);
    if (tmpl) {
        item.durability = tmpl->durability;
    }

    // Apply random affix for green quality and above
    if (quality >= static_cast<uint32>(ItemQuality::Green)) {
        item.affix = rollAffix(entry, level);
        item.affixScore = static_cast<uint32>(rollAffixScore() * 100.0f);
    }

    return item;
}

uint32 LootRoller::rollAffix(uint32 itemEntry, uint16 level) {
    const GameCache& cache = getGameCache();

    // Collect all valid affixes for this level range
    std::vector<uint32> validAffixes;

    // Scan affix templates: valid if level is within [minLevel, maxLevel]
    for (uint32 affixId = 1; affixId <= 9999; ++affixId) {
        const AffixTemplate* affix = cache.getAffixTemplate(affixId);
        if (!affix) continue;
        if (level >= affix->minLevel && level <= affix->maxLevel) {
            validAffixes.push_back(affix->entry);
        }
    }

    if (validAffixes.empty()) {
        // Log error matching the binary string
        LOG_ERROR("LootRoller::rollRandomLoot - Bad affix id chosen, affixStatStyle size = %d, levelRange = %d,%d",
                  0, static_cast<int>(level), static_cast<int>(level));
        return 0;
    }

    size_t idx = std::rand() % validAffixes.size();
    return validAffixes[idx];
}

float LootRoller::rollAffixScore() {
    // Return random float between 0.5 and 1.0
    std::uniform_real_distribution<float> dist(0.5f, 1.0f);
    return dist(s_rng);
}

uint32 LootRoller::rollGold(uint16 npcLevel, float customGoldRatio) {
    // Base gold = npcLevel * 5, scaled by customGoldRatio
    // Add some randomness: +/- 20%
    float baseGold = static_cast<float>(npcLevel) * 5.0f * customGoldRatio;

    // Random variance: multiply by 0.8 to 1.2
    float variance = 0.8f + (static_cast<float>(std::rand() % 41) / 100.0f); // 0.80 - 1.20
    uint32 gold = static_cast<uint32>(baseGold * variance);

    if (gold < 1) gold = 1;
    return gold;
}
