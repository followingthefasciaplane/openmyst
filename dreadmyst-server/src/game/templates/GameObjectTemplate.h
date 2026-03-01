#pragma once
#include "core/Types.h"
#include <string>

// Game object template from game.db gameobject_template
// Exact columns verified from binary string at 0x0057f6b0
struct GameObjectTemplate {
    uint32 entry         = 0;
    uint32 flags         = 0;
    uint32 requiredItem  = 0;
    uint32 requiredQuest = 0;
    uint32 type          = 0;   // GameObjType enum
    int32  data[MAX_GAMEOBJ_DATA] = {};
};

// Game object spawn from game.db gameobject
// Exact columns verified from binary string at 0x0057f5c0
struct GameObjectSpawn {
    uint32 guid       = 0;
    uint32 entry      = 0;
    uint32 map        = 0;
    float  positionX  = 0.0f;
    float  positionY  = 0.0f;
    uint32 respawn    = 0;
    uint32 state      = 0;
};

// Gossip text entry from game.db gossip
// Exact columns verified from binary string at 0x0057f898
struct GossipEntry {
    uint32 entry    = 0;
    uint32 gossipId = 0;
    std::string text;
    // Up to 3 conditions
    struct Condition {
        uint32 type   = 0;
        int32  value1 = 0;
        int32  value2 = 0;
        uint8  isTrue = 0;
    };
    Condition conditions[MAX_GOSSIP_CONDITIONS] = {};
};

// Gossip option from game.db gossip_option
// Exact columns verified from binary string at 0x0057fa50
struct GossipOption {
    uint32 entry           = 0;
    uint32 gossipId        = 0;
    std::string text;
    uint32 requiredNpcFlag = 0;
    uint32 clickNewGossip  = 0;
    uint32 clickScript     = 0;
    struct Condition {
        uint32 type   = 0;
        int32  value1 = 0;
        int32  value2 = 0;
        uint8  isTrue = 0;
    };
    Condition conditions[MAX_GOSSIP_CONDITIONS] = {};
};

// Loot entry from game.db loot
// Exact columns verified from binary string at 0x00580358
struct LootEntry {
    uint32 entry    = 0;
    uint32 lootId   = 0;
    uint32 item     = 0;
    float  chance   = 0.0f;
    uint32 countMin = 1;
    uint32 countMax = 1;
    struct Condition {
        uint32 type   = 0;
        int32  value1 = 0;
        int32  value2 = 0;
        uint8  isTrue = 0;
    };
    Condition conditions[MAX_LOOT_CONDITIONS] = {};
};

// Script entry from game.db scripts
// Exact columns verified from binary string at 0x00589190
struct ScriptEntry {
    uint32 entry    = 0;
    uint32 scriptId = 0;
    uint32 delay    = 0;
    uint32 command  = 0;   // ScriptCommand enum
    int32  data1    = 0;
    int32  data2    = 0;
    int32  data3    = 0;
    int32  data4    = 0;
    int32  data5    = 0;
};

// Dialog text from game.db dialog
// Exact columns verified from binary string at 0x0057eb20
struct DialogEntry {
    uint32 entry = 0;
    std::string text;
    uint32 sound = 0;
    uint32 type  = 0;
};

// World text string from MySQL world_texts
struct WorldText {
    uint32 id = 0;
    std::string text;
};
