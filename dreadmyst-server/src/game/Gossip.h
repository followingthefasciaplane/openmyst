#pragma once
#include "core/Types.h"
#include "game/templates/GameObjectTemplate.h"
#include <vector>

class ServerPlayer;
class ServerNpc;

// Gossip system for NPC dialog menus
// RTTI from binary: GP_Server_GossipMenu
namespace GossipSystem {
    // Show gossip menu for an NPC to a player
    void showGossipMenu(ServerPlayer* player, ServerNpc* npc);

    // Handle gossip option selection
    void handleGossipOption(ServerPlayer* player, ServerNpc* npc, uint32 optionIndex);

    // Check gossip conditions
    bool checkCondition(ServerPlayer* player, uint32 condType, int32 value1, int32 value2, bool isTrue);

    // Show vendor window
    void showVendorWindow(ServerPlayer* player, ServerNpc* npc);

    // Show quest giver window
    void showQuestGiverWindow(ServerPlayer* player, ServerNpc* npc);
}
