#pragma once
#include "core/Types.h"

class Session;
class GamePacket;

namespace PacketHandler {
    // Master dispatch - called from Session when packet arrives
    bool dispatch(Session* session, uint16 opcode, GamePacket& packet);

    // Individual handlers (matching binary dispatch)
    void handleTimeSync(Session* session, GamePacket& packet);
    void handleAuthenticate(Session* session, GamePacket& packet);
    void handleCharCreate(Session* session, GamePacket& packet);
    void handleEnterWorld(Session* session, GamePacket& packet);
    void handleRequestCharList(Session* session, GamePacket& packet);
    void handleDeleteChar(Session* session, GamePacket& packet);
    void handleChatMessage(Session* session, GamePacket& packet);
    void handleMovement(Session* session, GamePacket& packet);
    void handleTargetChange(Session* session, GamePacket& packet);
    void handleInventorySwap(Session* session, GamePacket& packet);
    void handleEquipmentSwap(Session* session, GamePacket& packet);
    void handleUseItem(Session* session, GamePacket& packet);
    void handleBankDeposit(Session* session, GamePacket& packet);
    void handleBankSwap(Session* session, GamePacket& packet);
    void handleVendorBuy(Session* session, GamePacket& packet);
    void handleGuildMotd(Session* session, GamePacket& packet);
    void handleNpcInteract(Session* session, GamePacket& packet);
    void handleQuestAction(Session* session, GamePacket& packet);
    void handleGuildCreate(Session* session, GamePacket& packet);
    void handleGuildInvite(Session* session, GamePacket& packet);
    void handlePartyInvite(Session* session, GamePacket& packet);
    void handlePartyResponse(Session* session, GamePacket& packet);
    void handleCastSpell(Session* session, GamePacket& packet);
    void handleAutoAttack(Session* session, GamePacket& packet);
    void handleSetTarget(Session* session, GamePacket& packet);
    void handleLootAction(Session* session, GamePacket& packet);
    void handleTradeRequest(Session* session, GamePacket& packet);
    void handleGameObjInteract(Session* session, GamePacket& packet);
    void handleSpellCancel(Session* session, GamePacket& packet);
    void handleGossipSelect(Session* session, GamePacket& packet);
    void handleArenaQueue(Session* session, GamePacket& packet);
    void handleStatInvest(Session* session, GamePacket& packet);
    void handleGuildInviteResponse(Session* session, GamePacket& packet);
    void handleGuildLeave(Session* session, GamePacket& packet);
    void handleGuildKick(Session* session, GamePacket& packet);
    void handleGuildRankChange(Session* session, GamePacket& packet);
    void handlePartyChanges(Session* session, GamePacket& packet);
    void handlePartyLeave(Session* session, GamePacket& packet);
    void handleMailSend(Session* session, GamePacket& packet);
    void handleMailAction(Session* session, GamePacket& packet);
    void handleWaypointTeleport(Session* session, GamePacket& packet);
    void handleInspect(Session* session, GamePacket& packet);
    void handleCombineItems(Session* session, GamePacket& packet);
    void handleLogin(Session* session, GamePacket& packet);
}
