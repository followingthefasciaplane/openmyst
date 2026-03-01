#pragma once
#include "core/Types.h"
#include "game/templates/GameObjectTemplate.h"
#include <vector>
#include <string>

class ServerPlayer;
class ServerNpc;
class ServerUnit;

// Script command types from binary RTTI:
// ScriptCmd_CastSpell, ScriptCmd_KillCredit, ScriptCmd_LocateNpc,
// ScriptCmd_OpenBank, ScriptCmd_PromptRespec, ScriptCmd_QueueArena,
// ScriptCmd_Talk
//
// Script entries loaded from game.db `scripts` table

namespace ScriptSystem {
    // Execute a script by entry
    void executeScript(uint32 scriptEntry, ServerPlayer* player, ServerUnit* source);

    // Execute a single script command
    void executeCommand(const ScriptEntry& cmd, ServerPlayer* player, ServerUnit* source);

    // Individual command handlers (matching binary RTTI classes)
    void cmdTalk(ServerPlayer* player, ServerUnit* source, int32 data1, int32 data2, int32 data3);
    void cmdCastSpell(ServerPlayer* player, ServerUnit* source, int32 spellId, int32 data2, int32 data3);
    void cmdKillCredit(ServerPlayer* player, int32 npcEntry, int32 data2, int32 data3);
    void cmdLocateNpc(ServerPlayer* player, int32 npcEntry, int32 data2, int32 data3);
    void cmdOpenBank(ServerPlayer* player);
    void cmdPromptRespec(ServerPlayer* player);
    void cmdQueueArena(ServerPlayer* player, int32 data1, int32 data2, int32 data3);
}
