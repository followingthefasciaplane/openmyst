#pragma once

#include "GameDefines.h"
#include "StlBuffer.h"
#include <string>
#include <cstdint>

// ============================================================================
//  Client -> Server packet structures (CMSG)
//  Binary-verified from Dreadmyst.exe r1189 RTTI strings
//
//  Convention: All fields use m_ prefix. build() writes to StlBuffer.
//  Pattern: StlBuffer buf; pk.build(buf); connector->send(opcode, buf);
// ============================================================================

// Base for client packets
struct GamePacketClient
{
    virtual ~GamePacketClient() = default;
    virtual void build(StlBuffer& buf) const {}
};

// CMSG_AUTHENTICATE
struct GP_Client_Authenticate : public GamePacketClient
{
    std::string m_username;
    std::string m_password;
    int32_t     m_version = 0;
};

// CMSG_CHAR_CREATE
struct GP_Client_CharCreate : public GamePacketClient
{
    std::string m_name;
    int32_t     m_classId  = 0;
    int32_t     m_gender   = 0;
    int32_t     m_portrait = 0;
};

// CMSG_CHAR_DELETE
struct GP_Client_CharDelete : public GamePacketClient
{
    int32_t m_characterGuid = 0;
};

// CMSG_ENTER_WORLD
struct GP_Client_EnterWorld : public GamePacketClient
{
    int32_t m_characterGuid = 0;
};

// CMSG_REQUEST_MOVE
struct GP_Client_RequestMove : public GamePacketClient
{
    float m_destX = 0.0f;
    float m_destY = 0.0f;
};

// CMSG_CAST_SPELL
struct GP_Client_CastSpell : public GamePacketClient
{
    int32_t m_spellId    = 0;
    int32_t m_targetGuid = 0;
    float   m_targetX    = 0.0f;
    float   m_targetY    = 0.0f;
};

// CMSG_CHAT_MSG
struct GP_Client_ChatMsg : public GamePacketClient
{
    uint8_t     m_chatType = 0;
    std::string m_message;
    std::string m_target;      // whisper target name
};

// CMSG_EQUIP_ITEM
struct GP_Client_EquipItem : public GamePacketClient
{
    int32_t m_slotId = 0;
    int32_t m_entry  = 0;
};

// CMSG_USE_ITEM
struct GP_Client_UseItem : public GamePacketClient
{
    int32_t m_slotId     = 0;
    int32_t m_targetGuid = 0;
};

// CMSG_REPAIR
struct GP_Client_Repair : public GamePacketClient
{
    int32_t m_npcGuid = 0;
    int32_t m_slotId  = -1;   // -1 = repair all
};

// CMSG_MOVE_INVENTORY_TO_BANK
struct GP_Client_MoveInventoryToBank : public GamePacketClient
{
    int32_t m_inventorySlot = 0;
    int32_t m_bankSlot      = 0;
};

// CMSG_UNBANK_ITEM
struct GP_Client_UnBankItem : public GamePacketClient
{
    int32_t m_bankSlot      = 0;
    int32_t m_inventorySlot = 0;
};

// CMSG_INTERACT_NPC
struct GP_Client_InteractNpc : public GamePacketClient
{
    int32_t m_npcGuid = 0;
};

// CMSG_GOSSIP_SELECT
struct GP_Client_GossipSelect : public GamePacketClient
{
    int32_t m_npcGuid   = 0;
    int32_t m_optionId  = 0;
};

// CMSG_QUEST_ACCEPT
struct GP_Client_QuestAccept : public GamePacketClient
{
    int32_t m_questId = 0;
};

// CMSG_QUEST_ABANDON
struct GP_Client_QuestAbandon : public GamePacketClient
{
    int32_t m_questId = 0;
};

// CMSG_QUEST_COMPLETE
struct GP_Client_QuestComplete : public GamePacketClient
{
    int32_t m_questId = 0;
    int32_t m_npcGuid = 0;
};

// CMSG_GUILD_CREATE
struct GP_Client_GuildCreate : public GamePacketClient
{
    std::string m_guildName;
};

// CMSG_GUILD_INVITE
struct GP_Client_GuildInvite : public GamePacketClient
{
    std::string m_targetName;
};

// CMSG_GUILD_INVITE_RESPONSE
struct GP_Client_GuildInviteResponse : public GamePacketClient
{
    bool m_accepted = false;
};

// CMSG_GUILD_ROSTER_REQUEST
struct GP_Client_GuildRosterRequest : public GamePacketClient
{
};

// CMSG_GUILD_LEAVE
struct GP_Client_GuildLeave : public GamePacketClient
{
};

// CMSG_GUILD_PROMOTE / DEMOTE
struct GP_Client_GuildRankChange : public GamePacketClient
{
    std::string m_targetName;
    uint8_t     m_newRank = 0;
};

// CMSG_DUEL_REQUEST
struct GP_Client_DuelRequest : public GamePacketClient
{
    int32_t m_targetGuid = 0;
};

// CMSG_DUEL_RESPONSE
struct GP_Client_DuelResponse : public GamePacketClient
{
    bool m_accepted = false;
};

// CMSG_PARTY_INVITE
struct GP_Client_PartyInvite : public GamePacketClient
{
    std::string m_targetName;
};

// CMSG_PARTY_INVITE_RESPONSE
struct GP_Client_PartyInviteResponse : public GamePacketClient
{
    bool m_accepted = false;
};

// CMSG_PARTY_LEAVE
struct GP_Client_PartyLeave : public GamePacketClient
{
};

// CMSG_TRADE_REQUEST
struct GP_Client_TradeRequest : public GamePacketClient
{
    int32_t m_targetGuid = 0;
};

// CMSG_TRADE_OFFER_ITEM
struct GP_Client_TradeOfferItem : public GamePacketClient
{
    int32_t m_slotId = 0;
};

// CMSG_TRADE_OFFER_GOLD
struct GP_Client_TradeOfferGold : public GamePacketClient
{
    int32_t m_amount = 0;
};

// CMSG_TRADE_CONFIRM
struct GP_Client_TradeConfirm : public GamePacketClient
{
};

// CMSG_TRADE_CANCEL
struct GP_Client_TradeCancel : public GamePacketClient
{
};

// CMSG_LOOT_ITEM
struct GP_Client_LootItem : public GamePacketClient
{
    int32_t m_objectGuid = 0;
    int32_t m_slotId     = 0;
};

// CMSG_REQUEST_RESPAWN
struct GP_Client_RequestRespawn : public GamePacketClient
{
    int32_t m_waypointId = 0;
};

// CMSG_UPDATE_ARENA_STATUS
struct GP_Client_UpdateArenaStatus : public GamePacketClient
{
    int32_t m_action = 0;   // 0=leave queue, 1=join queue, 2=query status
};

// CMSG_RESPEC
struct GP_Client_Respec : public GamePacketClient
{
    bool m_confirmed = false;
};

// CMSG_SOCKET_ITEM
struct GP_Client_SocketItem : public GamePacketClient
{
    int32_t m_itemSlotId = 0;
    int32_t m_gemSlotId  = 0;
};

// CMSG_EMPOWER_ITEM
struct GP_Client_EmpowerItem : public GamePacketClient
{
    int32_t m_itemSlotId = 0;
};

// CMSG_INSPECT
struct GP_Client_Inspect : public GamePacketClient
{
    int32_t m_targetGuid = 0;
};

// CMSG_BUY_VENDOR_ITEM
struct GP_Client_BuyVendorItem : public GamePacketClient
{
    int32_t m_npcGuid = 0;
    int32_t m_itemId  = 0;
    int32_t m_count   = 1;
};

// CMSG_SELL_ITEM
struct GP_Client_SellItem : public GamePacketClient
{
    int32_t m_npcGuid = 0;
    int32_t m_slotId  = 0;
};

// CMSG_WAYPOINT_TELEPORT
struct GP_Client_WaypointTeleport : public GamePacketClient
{
    int32_t m_waypointId = 0;
};

// CMSG_CHANNEL_CHANGE
struct GP_Client_ChannelChange : public GamePacketClient
{
    int32_t m_channelId = 0;
};
