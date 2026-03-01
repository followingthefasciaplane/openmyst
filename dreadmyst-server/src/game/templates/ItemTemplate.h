#pragma once
#include "core/Types.h"
#include <string>

// Item stat slot
struct ItemStatSlot {
    uint32 type  = 0;   // StatType enum
    int32  value = 0;
};

// Item template loaded from game.db item_template table
// Exact columns verified from binary string at 0x0058ccf8
struct ItemTemplate {
    uint32 entry            = 0;
    uint32 sortEntry        = 0;
    uint32 requiredLevel    = 0;
    uint32 weaponType       = 0;
    uint32 armorType        = 0;
    uint32 equipType        = 0;   // EquipType enum
    uint32 quality          = 0;   // ItemQuality enum
    uint32 itemLevel        = 0;
    uint32 durability       = 0;
    uint32 sellPrice        = 0;
    uint32 stackCount       = 1;
    uint32 weaponMaterial   = 0;
    uint32 numSockets       = 0;
    uint32 requiredClass    = 0;
    uint32 questOffer       = 0;
    uint32 flags            = 0;
    uint32 generated        = 0;
    float  buyCostRatio     = 1.0f;
    uint32 spells[MAX_ITEM_SPELLS] = {};
    ItemStatSlot stats[MAX_ITEM_STATS] = {};
};

// Affix (random enchantment) template from game.db affix_template
// Exact columns verified from binary string at 0x0058d000
struct AffixTemplate {
    uint32 entry    = 0;
    uint32 minLevel = 0;
    uint32 maxLevel = 0;
    ItemStatSlot stats[MAX_AFFIX_STATS] = {};
};

// Gem definition from game.db item_gems
// Exact columns verified from binary string at 0x005802c8
struct GemTemplate {
    uint32 gemId      = 0;
    uint32 quality    = 0;
    uint32 stat1      = 0;
    int32  stat1Amount = 0;
    uint32 stat2      = 0;
    int32  stat2Amount = 0;
};

// Item instance (runtime, stored in player inventory/equipment/bank)
struct ItemInstance {
    uint32 entry         = 0;   // Item template ID
    uint32 affix         = 0;   // Random enchantment ID
    uint32 affixScore    = 0;   // Affix roll quality
    uint32 gem1          = 0;
    uint32 gem2          = 0;
    uint32 gem3          = 0;
    uint32 gem4          = 0;
    uint32 enchantLevel  = 0;
    uint32 count         = 1;
    uint8  soulbound     = 0;
    uint32 durability    = 0;

    bool isEmpty() const { return entry == 0; }
    void clear() { *this = ItemInstance{}; }
};

// Combine recipe from game.db combine_items
// Exact query: SELECT item_id, item_id_2, output_item FROM combine_items
struct CombineRecipe {
    uint32 itemId1    = 0;
    uint32 itemId2    = 0;
    uint32 outputItem = 0;
};

// Material drop chance tables
struct MaterialChance {
    uint32 level    = 0;
    uint32 type     = 0;   // armor_type or weapon_material
    float  chance   = 0.0f;
};
