#pragma once
#include "core/Types.h"
#include <string>

// Map definition from game.db map table
// Exact columns verified from binary string at 0x00585e88
struct MapTemplate {
    uint32 id          = 0;
    std::string name;
    float  startX      = 0.0f;
    float  startY      = 0.0f;
    float  startO      = 0.0f;
    uint8  losVision   = 0;
};

// Zone template from game.db zone_template
// Exact columns verified from binary string at 0x0058c6d8
struct ZoneTemplate {
    uint32 id   = 0;
    uint32 type = 0;
};

// Dungeon template from game.db dungeon_template
// Exact columns verified from binary string at 0x00580178
struct DungeonTemplate {
    uint32 map           = 0;
    uint32 maxPlayers    = 0;
    uint32 requiredLevel = 0;
    uint32 requiredItem  = 0;
    uint32 requiredQuest = 0;
};

// Arena template from game.db arena_template
// Exact columns verified from binary string at 0x0057d6c8
struct ArenaTemplate {
    uint32 map           = 0;
    float  player1X      = 0.0f;
    float  player1Y      = 0.0f;
    float  player2X      = 0.0f;
    float  player2Y      = 0.0f;
    float  player3X      = 0.0f;
    float  player3Y      = 0.0f;
    float  player4X      = 0.0f;
    float  player4Y      = 0.0f;
    float  player1XStart = 0.0f;
    float  player1YStart = 0.0f;
    float  player2XStart = 0.0f;
    float  player2YStart = 0.0f;
    float  player3XStart = 0.0f;
    float  player3YStart = 0.0f;
    float  player4XStart = 0.0f;
    float  player4YStart = 0.0f;
};

// Graveyard from game.db graveyard
// Exact columns verified from binary string at 0x0057fc30
struct Graveyard {
    uint32 map       = 0;
    uint32 respawnMap = 0;
    float  respawnX  = 0.0f;
    float  respawnY  = 0.0f;
};

// Teleport names from MySQL teleport_names
struct TeleportLocation {
    std::string name;
    float  targetX    = 0.0f;
    float  targetY    = 0.0f;
    uint32 targetMapId = 0;
};
