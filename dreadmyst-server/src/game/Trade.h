#pragma once
#include "core/Types.h"
#include "game/templates/ItemTemplate.h"
#include <array>

class ServerPlayer;

// Trade session between two players
// RTTI from binary: GP_Client_OpenTradeWith, GP_Server_TradeUpdate
class Trade {
public:
    static const uint32 MAX_TRADE_SLOTS = 6;

    Trade(ServerPlayer* player1, ServerPlayer* player2);
    ~Trade() = default;

    ServerPlayer* getPlayer1() const { return m_player1; }
    ServerPlayer* getPlayer2() const { return m_player2; }

    // Set item in trade slot (from inventory slot)
    bool setItem(ServerPlayer* player, uint32 tradeSlot, uint32 inventorySlot);
    bool removeItem(ServerPlayer* player, uint32 tradeSlot);

    // Set gold offer
    void setGold(ServerPlayer* player, uint32 amount);

    // Accept/decline
    void accept(ServerPlayer* player);
    void decline(ServerPlayer* player);
    bool isBothAccepted() const { return m_player1Accepted && m_player2Accepted; }

    // Execute the trade
    bool executeTrade();

    // Cancel
    void cancel();

    // Send trade window updates
    void sendTradeUpdate(ServerPlayer* target);

    bool isActive() const { return m_active; }

private:
    ServerPlayer* getOtherPlayer(ServerPlayer* player) const;

    ServerPlayer* m_player1 = nullptr;
    ServerPlayer* m_player2 = nullptr;

    std::array<ItemInstance, MAX_TRADE_SLOTS> m_player1Items = {};
    std::array<ItemInstance, MAX_TRADE_SLOTS> m_player2Items = {};
    std::array<uint32, MAX_TRADE_SLOTS> m_player1InvSlots = {};
    std::array<uint32, MAX_TRADE_SLOTS> m_player2InvSlots = {};

    uint32 m_player1Gold = 0;
    uint32 m_player2Gold = 0;
    bool   m_player1Accepted = false;
    bool   m_player2Accepted = false;
    bool   m_active = true;
};
