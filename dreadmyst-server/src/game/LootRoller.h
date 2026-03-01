#pragma once
#include "core/Types.h"
#include "game/templates/ItemTemplate.h"
#include <vector>

class GameCache;
class ServerNpc;

struct LootItem {
    ItemInstance item;
    float       chance  = 0.0f;
    uint32      minCount = 1;
    uint32      maxCount = 1;
};

class LootRoller {
public:
    static LootRoller& instance();

    // Roll loot for an NPC kill
    std::vector<ItemInstance> rollNpcLoot(const ServerNpc* npc, uint16 killerLevel);

    // Roll loot from a loot table entry
    std::vector<ItemInstance> rollLootTable(uint32 lootId, uint16 rollerLevel);

    // Roll a random world loot item for a level range
    ItemInstance rollRandomLoot(int32 quality, uint16 level);

    // Generate random affix for an item
    uint32 rollAffix(uint32 itemEntry, uint16 level);
    float  rollAffixScore();

    // Roll gold amount
    uint32 rollGold(uint16 npcLevel, float customGoldRatio);

private:
    LootRoller() = default;

    ItemInstance createItemWithAffix(uint32 entry, uint16 level, uint32 quality);
    bool rollChance(float chance);
    uint32 rollRange(uint32 min, uint32 max);
};
