#include "game/Script.h"
#include "game/ServerPlayer.h"
#include "game/ServerNpc.h"
#include "game/ServerMap.h"
#include "game/GameCache.h"
#include "game/Spell.h"
#include "game/Arena.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"
#include "core/Log.h"

extern GameCache& getGameCache();

namespace ScriptSystem {

void executeScript(uint32 scriptEntry, ServerPlayer* player, ServerUnit* source) {
    const auto& scripts = getGameCache().getScriptEntries(scriptEntry);
    for (const auto& cmd : scripts) {
        executeCommand(cmd, player, source);
    }
}

void executeCommand(const ScriptEntry& cmd, ServerPlayer* player, ServerUnit* source) {
    switch (static_cast<ScriptCommand>(cmd.command)) {
    case ScriptCommand::Talk:
        cmdTalk(player, source, cmd.data1, cmd.data2, cmd.data3);
        break;
    case ScriptCommand::CastSpell:
        cmdCastSpell(player, source, cmd.data1, cmd.data2, cmd.data3);
        break;
    case ScriptCommand::KillCredit:
        cmdKillCredit(player, cmd.data1, cmd.data2, cmd.data3);
        break;
    case ScriptCommand::LocateNpc:
        cmdLocateNpc(player, cmd.data1, cmd.data2, cmd.data3);
        break;
    case ScriptCommand::OpenBank:
        cmdOpenBank(player);
        break;
    case ScriptCommand::PromptRespec:
        cmdPromptRespec(player);
        break;
    case ScriptCommand::QueueArena:
        cmdQueueArena(player, cmd.data1, cmd.data2, cmd.data3);
        break;
    default:
        LOG_DEBUG("Unknown script command %d in script entry %d",
                  cmd.command, cmd.entry);
        break;
    }
}

void cmdTalk(ServerPlayer* player, ServerUnit* source, int32 data1, int32 data2, int32 data3) {
    // data1 = dialog entry
    const DialogEntry* dialog = nullptr;
    for (const auto& d : getGameCache().getDialogs()) {
        if (d.entry == static_cast<uint32>(data1)) { dialog = &d; break; }
    }
    if (!dialog) {
        LOG_WARN("Script talk: dialog entry %d not found", data1);
        return;
    }

    GamePacket pkt;
    pkt.writeUint16(SMSG_CHAT_MSG);
    pkt.writeUint8(5); // System channel
    if (source) {
        pkt.writeString(source->getName());
    } else {
        pkt.writeString("System");
    }
    pkt.writeString(dialog->text);
    player->sendPacket(pkt);
}

void cmdCastSpell(ServerPlayer* player, ServerUnit* source, int32 spellId, int32 data2, int32 data3) {
    const SpellTemplate* tmpl = getGameCache().getSpellTemplate(static_cast<uint32>(spellId));
    if (!tmpl) {
        LOG_WARN("Script castspell: spell %d not found", spellId);
        return;
    }

    ServerUnit* caster = source ? source : static_cast<ServerUnit*>(player);
    Spell spell(caster, tmpl);
    spell.setTarget(player->getGuid());
    SpellCastResult result = spell.validate();
    if (result == SpellCastResult::Success) {
        spell.cast();
    }
}

void cmdKillCredit(ServerPlayer* player, int32 npcEntry, int32 data2, int32 data3) {
    // Update quest objectives that require killing this NPC entry
    const auto& quests = player->getQuests();
    for (const auto& qs : quests) {
        if (qs.rewarded) continue;
        const QuestTemplate* qt = getGameCache().getQuestTemplate(qs.questEntry);
        if (!qt) continue;
        for (int i = 0; i < 4; i++) {
            if (qt->reqNpc[i] == static_cast<uint32>(npcEntry)) {
                player->updateQuestObjective(qs.questEntry, i, 1);
            }
        }
    }
}

void cmdLocateNpc(ServerPlayer* player, int32 npcEntry, int32 data2, int32 data3) {
    // Send NPC location to player (mark on map)
    ServerMap* map = player->getMap();
    if (!map) return;

    GamePacket pkt;
    pkt.writeUint16(SMSG_MARK_NPCS_ON_MAP);
    pkt.writeUint32(1); // count

    auto npcs = map->getNpcs();
    for (auto& npc : npcs) {
        if (npc->getEntry() == static_cast<uint32>(npcEntry)) {
            pkt.writeUint32(npc->getGuid());
            pkt.writeFloat(npc->getPositionX());
            pkt.writeFloat(npc->getPositionY());
            player->sendPacket(pkt);
            return;
        }
    }
}

void cmdOpenBank(ServerPlayer* player) {
    // Send bank contents to player
    GamePacket pkt;
    pkt.writeUint16(SMSG_BANK);
    for (uint32 i = 0; i < MAX_BANK_SLOTS; i++) {
        const ItemInstance& item = player->getBankItem(i);
        pkt.writeUint32(item.entry);
        if (item.entry != 0) {
            pkt.writeUint32(item.affix);
            pkt.writeFloat(item.affixScore);
            pkt.writeUint32(item.gem1);
            pkt.writeUint32(item.gem2);
            pkt.writeUint32(item.gem3);
            pkt.writeUint32(item.gem4);
            pkt.writeUint32(item.enchantLevel);
            pkt.writeUint32(item.count);
            pkt.writeUint8(item.soulbound ? 1 : 0);
            pkt.writeInt32(item.durability);
        }
    }
    player->sendPacket(pkt);
}

void cmdPromptRespec(ServerPlayer* player) {
    // Send respec prompt UI to player
    GamePacket pkt;
    pkt.writeUint16(SMSG_GOSSIP_MENU);
    pkt.writeUint32(0); // special respec gossip
    pkt.writeUint8(1);  // option count
    pkt.writeString("Respec Stats");
    pkt.writeUint32(0); // option data
    player->sendPacket(pkt);
}

void cmdQueueArena(ServerPlayer* player, int32 data1, int32 data2, int32 data3) {
    ArenaMgr::instance().queueSolo(player);
}

} // namespace ScriptSystem
