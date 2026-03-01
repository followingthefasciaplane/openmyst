#pragma once
#include "core/Types.h"

// Player class base stats per level from game.db player_class_stats
// Exact columns verified from binary string at 0x005873d0
struct ClassStats {
    uint32 classId      = 0;
    uint32 level        = 0;
    uint32 hp           = 0;
    uint32 mana         = 0;
    uint32 strength     = 0;
    uint32 agility      = 0;
    uint32 willpower    = 0;
    uint32 intelligence = 0;
    uint32 courage      = 0;
};

// XP requirements per level from game.db player_exp_levels
// Exact columns verified from binary string at 0x00587de0
struct ExpLevel {
    uint32 level   = 0;
    uint32 exp     = 0;
    uint32 killExp = 0;
};

// Starting item per class from game.db player_create_item
// Exact columns verified from binary string at 0x00587598
struct CreateItem {
    uint32 classId = 0;
    uint32 item    = 0;
    uint32 count   = 1;
};

// Starting spell per class from game.db player_create_spell
// Exact columns verified from binary string at 0x0058c1d0
struct CreateSpell {
    uint32 classId = 0;
    uint32 spell   = 0;
};

// Starting known waypoints from game.db player_create_known_waypoints
// Exact columns verified from binary string at 0x005875e0
struct CreateKnownWaypoint {
    uint32 entry      = 0;
    uint32 objectGuid = 0;
};

// Desirable stats per class for AI loot from game.db player_desirable_stats
// Exact columns verified from binary string at 0x0058c120
struct DesirableStat {
    uint32 classId = 0;
    uint32 statId  = 0;
};

// Desirable armor per class/level from game.db player_desirable_armor
// Exact columns verified from binary string at 0x0058c158
struct DesirableArmor {
    uint32 level     = 0;
    uint32 classId   = 0;
    uint32 armorType = 0;
};
