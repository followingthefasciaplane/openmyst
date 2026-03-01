#include "game/Trade.h"
#include "game/ServerPlayer.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"
#include "core/Log.h"

Trade::Trade(ServerPlayer* player1, ServerPlayer* player2)
    : m_player1(player1), m_player2(player2) {
    m_player1InvSlots.fill(0xFFFFFFFF);
    m_player2InvSlots.fill(0xFFFFFFFF);
}

ServerPlayer* Trade::getOtherPlayer(ServerPlayer* player) const {
    return (player == m_player1) ? m_player2 : m_player1;
}

bool Trade::setItem(ServerPlayer* player, uint32 tradeSlot, uint32 inventorySlot) {
    if (!m_active || tradeSlot >= MAX_TRADE_SLOTS) return false;

    const ItemInstance& item = player->getInventoryItem(inventorySlot);
    if (item.entry == 0) return false;

    // Reset accept state when items change
    m_player1Accepted = false;
    m_player2Accepted = false;

    if (player == m_player1) {
        m_player1Items[tradeSlot] = item;
        m_player1InvSlots[tradeSlot] = inventorySlot;
    } else {
        m_player2Items[tradeSlot] = item;
        m_player2InvSlots[tradeSlot] = inventorySlot;
    }

    sendTradeUpdate(m_player1);
    sendTradeUpdate(m_player2);
    return true;
}

bool Trade::removeItem(ServerPlayer* player, uint32 tradeSlot) {
    if (!m_active || tradeSlot >= MAX_TRADE_SLOTS) return false;

    m_player1Accepted = false;
    m_player2Accepted = false;

    if (player == m_player1) {
        m_player1Items[tradeSlot] = {};
        m_player1InvSlots[tradeSlot] = 0xFFFFFFFF;
    } else {
        m_player2Items[tradeSlot] = {};
        m_player2InvSlots[tradeSlot] = 0xFFFFFFFF;
    }

    sendTradeUpdate(m_player1);
    sendTradeUpdate(m_player2);
    return true;
}

void Trade::setGold(ServerPlayer* player, uint32 amount) {
    if (!m_active) return;

    if (amount > player->getMoney()) {
        amount = player->getMoney();
    }

    m_player1Accepted = false;
    m_player2Accepted = false;

    if (player == m_player1) {
        m_player1Gold = amount;
    } else {
        m_player2Gold = amount;
    }

    sendTradeUpdate(m_player1);
    sendTradeUpdate(m_player2);
}

void Trade::accept(ServerPlayer* player) {
    if (!m_active) return;

    if (player == m_player1) {
        m_player1Accepted = true;
    } else {
        m_player2Accepted = true;
    }

    if (isBothAccepted()) {
        executeTrade();
    }
}

void Trade::decline(ServerPlayer* player) {
    cancel();
}

bool Trade::executeTrade() {
    if (!m_active) return false;

    // Verify both players have enough inventory space
    int p1FreeSlots = 0, p2FreeSlots = 0;
    int p1ItemCount = 0, p2ItemCount = 0;

    for (uint32 i = 0; i < MAX_TRADE_SLOTS; i++) {
        if (m_player1Items[i].entry != 0) p1ItemCount++;
        if (m_player2Items[i].entry != 0) p2ItemCount++;
    }

    // Count free slots for each player (accounting for items they're giving away)
    for (uint32 i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (m_player1->getInventoryItem(i).entry == 0) p1FreeSlots++;
        if (m_player2->getInventoryItem(i).entry == 0) p2FreeSlots++;
    }

    // Player1 needs space for p2's items (minus slots freed by giving items)
    if (p1FreeSlots + p1ItemCount < p2ItemCount) {
        LOG_DEBUG("Trade failed: player1 not enough space");
        cancel();
        return false;
    }
    if (p2FreeSlots + p2ItemCount < p1ItemCount) {
        LOG_DEBUG("Trade failed: player2 not enough space");
        cancel();
        return false;
    }

    // Verify gold
    if (m_player1Gold > m_player1->getMoney() || m_player2Gold > m_player2->getMoney()) {
        cancel();
        return false;
    }

    // Remove items from player 1 inventory
    for (uint32 i = 0; i < MAX_TRADE_SLOTS; i++) {
        if (m_player1Items[i].entry != 0 && m_player1InvSlots[i] != 0xFFFFFFFF) {
            m_player1->removeInventoryItem(m_player1InvSlots[i], m_player1Items[i].count);
        }
    }

    // Remove items from player 2 inventory
    for (uint32 i = 0; i < MAX_TRADE_SLOTS; i++) {
        if (m_player2Items[i].entry != 0 && m_player2InvSlots[i] != 0xFFFFFFFF) {
            m_player2->removeInventoryItem(m_player2InvSlots[i], m_player2Items[i].count);
        }
    }

    // Give player 1's items to player 2
    for (uint32 i = 0; i < MAX_TRADE_SLOTS; i++) {
        if (m_player1Items[i].entry != 0) {
            m_player2->addItemToInventory(m_player1Items[i]);
        }
    }

    // Give player 2's items to player 1
    for (uint32 i = 0; i < MAX_TRADE_SLOTS; i++) {
        if (m_player2Items[i].entry != 0) {
            m_player1->addItemToInventory(m_player2Items[i]);
        }
    }

    // Exchange gold
    if (m_player1Gold > 0) {
        m_player1->addMoney(-(int32)m_player1Gold);
        m_player2->addMoney((int32)m_player1Gold);
    }
    if (m_player2Gold > 0) {
        m_player2->addMoney(-(int32)m_player2Gold);
        m_player1->addMoney((int32)m_player2Gold);
    }

    LOG_INFO("Trade completed between '%s' and '%s'",
             m_player1->getName().c_str(), m_player2->getName().c_str());

    m_active = false;
    return true;
}

void Trade::cancel() {
    if (!m_active) return;
    m_active = false;

    // Notify both players
    GamePacket pkt;
    pkt.writeUint16(SMSG_TRADE_UPDATE);
    pkt.writeUint8(0); // cancel flag
    m_player1->sendPacket(pkt);
    m_player2->sendPacket(pkt);
}

void Trade::sendTradeUpdate(ServerPlayer* target) {
    if (!m_active) return;

    auto& myItems = (target == m_player1) ? m_player1Items : m_player2Items;
    auto& otherItems = (target == m_player1) ? m_player2Items : m_player1Items;
    uint32 myGold = (target == m_player1) ? m_player1Gold : m_player2Gold;
    uint32 otherGold = (target == m_player1) ? m_player2Gold : m_player1Gold;

    GamePacket pkt;
    pkt.writeUint16(SMSG_TRADE_UPDATE);
    pkt.writeUint8(1); // update flag

    // My items
    for (uint32 i = 0; i < MAX_TRADE_SLOTS; i++) {
        pkt.writeUint32(myItems[i].entry);
        if (myItems[i].entry != 0) {
            pkt.writeUint32(myItems[i].count);
        }
    }
    pkt.writeUint32(myGold);

    // Other player's items
    for (uint32 i = 0; i < MAX_TRADE_SLOTS; i++) {
        pkt.writeUint32(otherItems[i].entry);
        if (otherItems[i].entry != 0) {
            pkt.writeUint32(otherItems[i].count);
        }
    }
    pkt.writeUint32(otherGold);

    // Accept states
    pkt.writeUint8(m_player1Accepted ? 1 : 0);
    pkt.writeUint8(m_player2Accepted ? 1 : 0);

    target->sendPacket(pkt);
}
