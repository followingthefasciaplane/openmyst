#include "game/Gossip.h"
#include "game/ServerPlayer.h"
#include "game/ServerNpc.h"
#include "game/GameCache.h"
#include "game/Script.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"
#include "core/Log.h"

extern GameCache& getGameCache();

namespace GossipSystem {

void showGossipMenu(ServerPlayer* player, ServerNpc* npc) {
    uint32 gossipMenuId = npc->getGossipMenuId();
    if (gossipMenuId == 0) {
        if (npc->isVendor()) {
            showVendorWindow(player, npc);
            return;
        }
        if (npc->isQuestGiver()) {
            showQuestGiverWindow(player, npc);
            return;
        }
        return;
    }

    const auto& entries = getGameCache().getGossipEntries(gossipMenuId);
    if (entries.empty()) {
        LOG_WARN("Gossip menu %u not found for NPC %u", gossipMenuId, npc->getEntry());
        return;
    }

    const GossipEntry& gossip = entries[0];

    // Check gossip conditions
    for (int i = 0; i < MAX_GOSSIP_CONDITIONS; i++) {
        if (gossip.conditions[i].type != 0) {
            if (!checkCondition(player, gossip.conditions[i].type,
                                gossip.conditions[i].value1,
                                gossip.conditions[i].value2,
                                gossip.conditions[i].isTrue)) {
                return;
            }
        }
    }

    const auto& options = getGameCache().getGossipOptions(gossipMenuId);

    GamePacket pkt;
    pkt.writeUint16(SMSG_GOSSIP_MENU);
    pkt.writeUint32(npc->getGuid());
    pkt.writeString(gossip.text);

    // Filter options by conditions
    std::vector<const GossipOption*> validOptions;
    for (const auto& opt : options) {
        bool valid = true;
        for (int i = 0; i < MAX_GOSSIP_CONDITIONS; i++) {
            if (opt.conditions[i].type != 0) {
                if (!checkCondition(player, opt.conditions[i].type,
                                    opt.conditions[i].value1,
                                    opt.conditions[i].value2,
                                    opt.conditions[i].isTrue)) {
                    valid = false;
                    break;
                }
            }
        }
        if (valid) {
            validOptions.push_back(&opt);
        }
    }

    pkt.writeUint8(static_cast<uint8>(validOptions.size()));
    for (const auto* opt : validOptions) {
        pkt.writeString(opt->text);
        pkt.writeUint32(opt->entry);
        pkt.writeUint32(opt->requiredNpcFlag);
    }

    player->sendPacket(pkt);
}

void handleGossipOption(ServerPlayer* player, ServerNpc* npc, uint32 optionIndex) {
    uint32 gossipMenuId = npc->getGossipMenuId();
    const auto& options = getGameCache().getGossipOptions(gossipMenuId);

    if (optionIndex >= options.size()) {
        LOG_WARN("Invalid gossip option %u for menu %u", optionIndex, gossipMenuId);
        return;
    }

    const GossipOption& opt = options[optionIndex];

    // If clickNewGossip is set, show a sub-menu
    if (opt.clickNewGossip > 0) {
        const auto& subEntries = getGameCache().getGossipEntries(opt.clickNewGossip);
        if (!subEntries.empty()) {
            GamePacket pkt;
            pkt.writeUint16(SMSG_GOSSIP_MENU);
            pkt.writeUint32(npc->getGuid());
            pkt.writeString(subEntries[0].text);
            pkt.writeUint8(0);
            player->sendPacket(pkt);
        }
        return;
    }

    // If clickScript is set, execute the script
    if (opt.clickScript > 0) {
        ScriptSystem::executeScript(opt.clickScript, player, npc);
        return;
    }

    // Check requiredNpcFlag for special actions
    if (opt.requiredNpcFlag != 0) {
        // Vendor flag
        if (npc->isVendor() && opt.requiredNpcFlag == 1) {
            showVendorWindow(player, npc);
            return;
        }
        // Quest giver flag
        if (npc->isQuestGiver() && opt.requiredNpcFlag == 2) {
            showQuestGiverWindow(player, npc);
            return;
        }
    }
}

bool checkCondition(ServerPlayer* player, uint32 condType, int32 value1, int32 value2, bool isTrue) {
    bool result = false;

    switch (static_cast<ConditionType>(condType)) {
    case ConditionType::None:
        result = true;
        break;

    case ConditionType::QuestDone:
        result = player->hasCompletedQuest(static_cast<uint32>(value1));
        break;

    case ConditionType::QuestActive:
        result = player->hasActiveQuest(static_cast<uint32>(value1));
        break;

    case ConditionType::HasItem:
        result = player->hasItem(static_cast<uint32>(value1),
                                 value2 > 0 ? static_cast<uint32>(value2) : 1);
        break;

    case ConditionType::Level:
        result = player->getLevel() >= static_cast<uint16>(value1);
        break;

    case ConditionType::Class:
        result = player->getClassId() == static_cast<uint8>(value1);
        break;

    default:
        LOG_DEBUG("Unknown condition type %u", condType);
        result = true;
        break;
    }

    return isTrue ? result : !result;
}

void showVendorWindow(ServerPlayer* player, ServerNpc* npc) {
    const auto& vendorItems = getGameCache().getVendorItems(npc->getEntry());

    GamePacket pkt;
    pkt.writeUint16(SMSG_VENDOR_LIST);
    pkt.writeUint32(npc->getGuid());
    pkt.writeUint32(static_cast<uint32>(vendorItems.size()));

    for (const auto& vi : vendorItems) {
        pkt.writeUint32(vi.item);
        pkt.writeUint32(vi.maxCount);
    }

    player->sendPacket(pkt);
}

void showQuestGiverWindow(ServerPlayer* player, ServerNpc* npc) {
    GamePacket pkt;
    pkt.writeUint16(SMSG_QUEST_LIST);
    pkt.writeUint32(npc->getGuid());

    // TODO: iterate quest templates and find ones offered by this NPC
    pkt.writeUint32(0); // quest count placeholder

    player->sendPacket(pkt);
}

} // namespace GossipSystem
