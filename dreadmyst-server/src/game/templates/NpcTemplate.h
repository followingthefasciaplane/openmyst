#pragma once
#include "core/Types.h"
#include <string>

// NPC spell slot (up to 4 per NPC)
struct NpcSpellSlot {
    uint32 spellId    = 0;
    uint32 chance     = 0;
    uint32 interval   = 0;
    uint32 cooldown   = 0;
    uint32 targetType = 0;
};

// NPC template loaded from game.db npc_template
// Exact columns verified from binary string at 0x00586350
struct NpcTemplate {
    uint32 entry              = 0;
    std::string name;
    uint32 modelId            = 0;
    float  modelScale         = 1.0f;
    uint32 minLevel           = 1;
    uint32 maxLevel           = 1;
    uint32 faction            = 0;
    uint32 type               = 0;
    uint32 npcFlags           = 0;   // NpcFlags enum
    uint32 gameFlags          = 0;
    uint32 gossipMenuId       = 0;
    uint32 movementType       = 0;   // MovementType enum
    uint32 pathId             = 0;
    uint32 aiType             = 0;   // AiType enum
    uint32 mechanicImmuneMask = 0;
    uint32 resistanceHoly     = 0;
    uint32 resistanceFrost    = 0;
    uint32 resistanceShadow   = 0;
    uint32 resistanceFire     = 0;
    uint32 strength           = 0;
    uint32 agility            = 0;
    uint32 intellect          = 0;
    uint32 willpower          = 0;
    uint32 courage            = 0;
    uint32 armor              = 0;
    uint32 health             = 0;
    uint32 mana               = 0;
    uint32 weaponValue        = 0;
    uint32 meleeSkill         = 0;
    uint32 rangedSkill        = 0;
    uint32 rangedWeaponValue  = 0;
    uint32 meleeSpeed         = 0;
    uint32 rangedSpeed        = 0;
    float  lootGreenChance    = 0.0f;
    float  lootBlueChance     = 0.0f;
    float  lootGoldChance     = 0.0f;
    float  lootPurpleChance   = 0.0f;
    uint32 customLoot         = 0;
    float  customGoldRatio    = 1.0f;
    uint32 shieldSkill        = 0;
    uint8  boolElite          = 0;
    uint8  boolBoss           = 0;
    float  leashRange         = 0.0f;
    uint32 spellPrimary       = 0;
    NpcSpellSlot spells[MAX_NPC_SPELLS] = {};
};

// NPC spawn data from game.db npc table
// Exact columns verified from binary string at 0x00586180
struct NpcSpawn {
    uint32 guid            = 0;
    uint32 entry           = 0;
    uint32 map             = 0;
    float  positionX       = 0.0f;
    float  positionY       = 0.0f;
    float  orientation     = 0.0f;
    uint32 pathId          = 0;
    uint32 respawnTime     = 0;
    uint32 movementType    = 0;
    float  wanderDistance   = 0.0f;
    uint8  deathState      = 0;
    uint8  callForHelp     = 0;
};

// NPC waypoint data from game.db npc_waypoints
// Exact columns verified from binary string at 0x005869b0
struct NpcWaypoint {
    uint32 id          = 0;
    uint32 point       = 0;
    float  positionX   = 0.0f;
    float  positionY   = 0.0f;
    float  orientation = 0.0f;
    uint8  run         = 0;
    uint32 waitTime    = 0;
    uint32 script      = 0;
};

// Vendor item from game.db npc_vendor
// Exact columns verified from binary string at 0x00586848
struct VendorItem {
    uint32 npcEntry        = 0;
    uint32 item            = 0;
    uint32 maxCount        = 0;
    uint32 restockCooldown = 0;
};

// Random vendor config from game.db npc_vendor_random
struct VendorRandom {
    uint32 npcEntry        = 0;
    uint32 minLevel        = 0;
    uint32 maxLevel        = 0;
    uint32 equipType       = 0;
    uint32 weaponType      = 0;
    uint32 armorPreference = 0;
    float  greenChance     = 0.0f;
    float  blueChance      = 0.0f;
    float  goldChance      = 0.0f;
    float  purpleChance    = 0.0f;
};

// NPC model junk loot mapping
struct NpcModelJunkLoot {
    uint32 modelId   = 0;
    uint32 itemEntry = 0;
};
