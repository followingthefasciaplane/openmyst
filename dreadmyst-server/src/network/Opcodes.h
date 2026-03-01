#pragma once
#include "core/Types.h"
#include <string>

// Opcodes reverse-engineered from the Dreadmyst client binary.
// The client validates opcode range [1, 150] (0x96).
//
// Confirmed values from binary analysis:
//   SMSG_VALIDATE         = 0x01
//   SMSG_CHARACTER_LIST   = 0x03
//   SMSG_SPELLBOOK        = 0x50
//   SMSG_CHAR_CREATE      = 0x51
//   SMSG_INSPECT          = 0x53
//
// Unconfirmed opcodes are assigned plausible values based on observed
// packet handler jump tables and control flow analysis.

constexpr uint16 MAX_OPCODE = 150;  // 0x96

// ============================================================================
//  Server -> Client (SMSG)
// ============================================================================
enum ServerOpcode : uint16 {
    SMSG_VALIDATE                 = 0x01,  // Auth validation response
    SMSG_CHARACTER_LIST           = 0x03,  // Character list for account
    SMSG_PLAYER                   = 0x04,  // Full player data snapshot
    SMSG_UNIT                     = 0x05,  // Unit (NPC / creature) data
    SMSG_UNIT_AURAS               = 0x06,  // Active auras on a unit
    SMSG_UNIT_SPLINE              = 0x07,  // Unit spline movement path
    SMSG_NPC                      = 0x08,  // NPC template / gossip data
    SMSG_OBJECT                   = 0x09,  // Generic world object
    SMSG_GAME_OBJECT              = 0x0A,  // Game object (chest, door, etc)
    SMSG_INVENTORY                = 0x0B,  // Full inventory contents
    SMSG_BANK                     = 0x0C,  // Bank contents
    SMSG_SPELLBOOK                = 0x50,  // Full spellbook
    SMSG_SPELLBOOK_UPDATE         = 0x0E,  // Single spell learned/removed
    SMSG_SPELL_GO                 = 0x0F,  // Spell cast visual + effect
    SMSG_COMBAT_MSG               = 0x10,  // Combat log message
    SMSG_CHAT_MSG                 = 0x11,  // Chat message
    SMSG_CHANNEL_INFO             = 0x12,  // Channel roster / info
    SMSG_QUEST_LIST               = 0x13,  // Quest log contents
    SMSG_GOSSIP_MENU              = 0x14,  // NPC gossip menu
    SMSG_OPEN_LOOT_WINDOW         = 0x15,  // Loot window contents
    SMSG_NOTIFY_ITEM_ADD          = 0x16,  // Item added notification
    SMSG_TRADE_UPDATE             = 0x17,  // Trade window update
    SMSG_OFFER_DUEL               = 0x18,  // Incoming duel request
    SMSG_OFFER_PARTY              = 0x19,  // Party invitation
    SMSG_PARTY_LIST               = 0x1A,  // Party member list
    SMSG_GUILD_ROSTER             = 0x1B,  // Full guild roster
    SMSG_GUILD_INVITE             = 0x1C,  // Guild invitation
    SMSG_GUILD_ADD_MEMBER         = 0x1D,  // Member joined guild
    SMSG_GUILD_REMOVE_MEMBER      = 0x1E,  // Member left guild
    SMSG_GUILD_NOTIFY_ROLE_CHANGE = 0x1F,  // Member rank changed
    SMSG_GUILD_ONLINE_STATUS      = 0x20,  // Member online/offline
    SMSG_AVAILABLE_WORLD_QUESTS   = 0x21,  // World quest board
    SMSG_MARK_NPCS_ON_MAP         = 0x22,  // Minimap NPC markers
    SMSG_QUERY_WAYPOINTS_RESPONSE = 0x23,  // Waypoint query result
    SMSG_SET_SUBNAME              = 0x24,  // Unit subname / title
    SMSG_PK_NOTIFY                = 0x25,  // PvP kill notification
    SMSG_VENDOR_LIST              = 0x26,  // Vendor item list

    // Game state update block (0x4B-0x4F range confirmed in handler table)
    SMSG_GAME_STATE_UPDATE_1      = 0x4B,
    SMSG_GAME_STATE_UPDATE_2      = 0x4C,
    SMSG_GAME_STATE_UPDATE_3      = 0x4D,
    SMSG_GAME_STATE_UPDATE_4      = 0x4E,
    SMSG_GAME_STATE_UPDATE_5      = 0x4F,

    SMSG_CHAR_CREATE              = 0x51,  // Character creation response
    SMSG_GAME_STATE               = 0x52,  // Full game state sync
    SMSG_INSPECT                  = 0x53,  // Inspect another player
};

// ============================================================================
//  Client -> Server (CMSG)
// ============================================================================
enum ClientOpcode : uint16 {
    CMSG_AUTHENTICATE             = 0x60,  // Login with auth token
    CMSG_CHAR_CREATE              = 0x61,  // Create new character
    CMSG_REQUEST_MOVE             = 0x62,  // Player movement input
    CMSG_CAST_SPELL               = 0x63,  // Cast a spell
    CMSG_CHAT_MSG                 = 0x64,  // Send chat message
    CMSG_EQUIP_ITEM               = 0x65,  // Equip an inventory item
    CMSG_USE_ITEM                 = 0x66,  // Use a consumable item
    CMSG_REPAIR                   = 0x67,  // Repair equipment at NPC
    CMSG_MOVE_INVENTORY_TO_BANK   = 0x68,  // Deposit item to bank
    CMSG_UNBANK_ITEM              = 0x69,  // Withdraw item from bank
    CMSG_LEVEL_UP                 = 0x6A,  // Allocate stat point on level up
    CMSG_OPEN_TRADE_WITH          = 0x6B,  // Initiate trade with player
    CMSG_PARTY_INVITE_MEMBER      = 0x6C,  // Invite player to party
    CMSG_PARTY_CHANGES            = 0x6D,  // Leave party / kick / etc
    CMSG_GUILD_CREATE             = 0x6E,  // Create new guild
    CMSG_GUILD_INVITE_MEMBER      = 0x6F,  // Invite player to guild
    CMSG_GUILD_INVITE_RESPONSE    = 0x70,  // Accept/decline guild invite
    CMSG_GUILD_PROMOTE_MEMBER     = 0x71,  // Promote guild member
    CMSG_GUILD_DEMOTE_MEMBER      = 0x72,  // Demote guild member
    CMSG_GUILD_MOTD               = 0x73,  // Set guild message of the day
    CMSG_UPDATE_ARENA_STATUS      = 0x74,  // Arena queue / status
    CMSG_SET_IGNORE_PLAYER        = 0x75,  // Add/remove ignore
    CMSG_REPORT_PLAYER            = 0x76,  // Report player
    CMSG_MOD_BAN_PLAYER           = 0x77,  // Moderator: ban
    CMSG_MOD_KICK_PLAYER          = 0x78,  // Moderator: kick
    CMSG_MOD_MUTE_PLAYER          = 0x79,  // Moderator: mute
    CMSG_MOD_WARN_PLAYER          = 0x7A,  // Moderator: warn
    CMSG_LOGIN                    = 0x7B,  // Login with username+password
};

// ============================================================================
//  Opcode name lookup (for logging / debug)
// ============================================================================
inline const char* opcodeToString(uint16 opcode) {
    switch (opcode) {
        // Server opcodes
        case SMSG_VALIDATE:                 return "SMSG_VALIDATE";
        case SMSG_CHARACTER_LIST:           return "SMSG_CHARACTER_LIST";
        case SMSG_PLAYER:                   return "SMSG_PLAYER";
        case SMSG_UNIT:                     return "SMSG_UNIT";
        case SMSG_UNIT_AURAS:              return "SMSG_UNIT_AURAS";
        case SMSG_UNIT_SPLINE:             return "SMSG_UNIT_SPLINE";
        case SMSG_NPC:                      return "SMSG_NPC";
        case SMSG_OBJECT:                   return "SMSG_OBJECT";
        case SMSG_GAME_OBJECT:             return "SMSG_GAME_OBJECT";
        case SMSG_INVENTORY:               return "SMSG_INVENTORY";
        case SMSG_BANK:                     return "SMSG_BANK";
        case SMSG_SPELLBOOK:               return "SMSG_SPELLBOOK";
        case SMSG_SPELLBOOK_UPDATE:        return "SMSG_SPELLBOOK_UPDATE";
        case SMSG_SPELL_GO:                return "SMSG_SPELL_GO";
        case SMSG_COMBAT_MSG:              return "SMSG_COMBAT_MSG";
        case SMSG_CHAT_MSG:                return "SMSG_CHAT_MSG";
        case SMSG_CHANNEL_INFO:            return "SMSG_CHANNEL_INFO";
        case SMSG_QUEST_LIST:              return "SMSG_QUEST_LIST";
        case SMSG_GOSSIP_MENU:             return "SMSG_GOSSIP_MENU";
        case SMSG_OPEN_LOOT_WINDOW:        return "SMSG_OPEN_LOOT_WINDOW";
        case SMSG_NOTIFY_ITEM_ADD:         return "SMSG_NOTIFY_ITEM_ADD";
        case SMSG_TRADE_UPDATE:            return "SMSG_TRADE_UPDATE";
        case SMSG_OFFER_DUEL:              return "SMSG_OFFER_DUEL";
        case SMSG_OFFER_PARTY:             return "SMSG_OFFER_PARTY";
        case SMSG_PARTY_LIST:              return "SMSG_PARTY_LIST";
        case SMSG_GUILD_ROSTER:            return "SMSG_GUILD_ROSTER";
        case SMSG_GUILD_INVITE:            return "SMSG_GUILD_INVITE";
        case SMSG_GUILD_ADD_MEMBER:        return "SMSG_GUILD_ADD_MEMBER";
        case SMSG_GUILD_REMOVE_MEMBER:     return "SMSG_GUILD_REMOVE_MEMBER";
        case SMSG_GUILD_NOTIFY_ROLE_CHANGE:return "SMSG_GUILD_NOTIFY_ROLE_CHANGE";
        case SMSG_GUILD_ONLINE_STATUS:     return "SMSG_GUILD_ONLINE_STATUS";
        case SMSG_AVAILABLE_WORLD_QUESTS:  return "SMSG_AVAILABLE_WORLD_QUESTS";
        case SMSG_MARK_NPCS_ON_MAP:        return "SMSG_MARK_NPCS_ON_MAP";
        case SMSG_QUERY_WAYPOINTS_RESPONSE:return "SMSG_QUERY_WAYPOINTS_RESPONSE";
        case SMSG_SET_SUBNAME:             return "SMSG_SET_SUBNAME";
        case SMSG_PK_NOTIFY:               return "SMSG_PK_NOTIFY";
        case SMSG_VENDOR_LIST:             return "SMSG_VENDOR_LIST";
        case SMSG_GAME_STATE_UPDATE_1:     return "SMSG_GAME_STATE_UPDATE_1";
        case SMSG_GAME_STATE_UPDATE_2:     return "SMSG_GAME_STATE_UPDATE_2";
        case SMSG_GAME_STATE_UPDATE_3:     return "SMSG_GAME_STATE_UPDATE_3";
        case SMSG_GAME_STATE_UPDATE_4:     return "SMSG_GAME_STATE_UPDATE_4";
        case SMSG_GAME_STATE_UPDATE_5:     return "SMSG_GAME_STATE_UPDATE_5";
        case SMSG_CHAR_CREATE:             return "SMSG_CHAR_CREATE";
        case SMSG_GAME_STATE:              return "SMSG_GAME_STATE";
        case SMSG_INSPECT:                 return "SMSG_INSPECT";

        // Client opcodes
        case CMSG_AUTHENTICATE:            return "CMSG_AUTHENTICATE";
        case CMSG_CHAR_CREATE:             return "CMSG_CHAR_CREATE";
        case CMSG_REQUEST_MOVE:            return "CMSG_REQUEST_MOVE";
        case CMSG_CAST_SPELL:              return "CMSG_CAST_SPELL";
        case CMSG_CHAT_MSG:                return "CMSG_CHAT_MSG";
        case CMSG_EQUIP_ITEM:              return "CMSG_EQUIP_ITEM";
        case CMSG_USE_ITEM:                return "CMSG_USE_ITEM";
        case CMSG_REPAIR:                  return "CMSG_REPAIR";
        case CMSG_MOVE_INVENTORY_TO_BANK:  return "CMSG_MOVE_INVENTORY_TO_BANK";
        case CMSG_UNBANK_ITEM:             return "CMSG_UNBANK_ITEM";
        case CMSG_LEVEL_UP:                return "CMSG_LEVEL_UP";
        case CMSG_OPEN_TRADE_WITH:         return "CMSG_OPEN_TRADE_WITH";
        case CMSG_PARTY_INVITE_MEMBER:     return "CMSG_PARTY_INVITE_MEMBER";
        case CMSG_PARTY_CHANGES:           return "CMSG_PARTY_CHANGES";
        case CMSG_GUILD_CREATE:            return "CMSG_GUILD_CREATE";
        case CMSG_GUILD_INVITE_MEMBER:     return "CMSG_GUILD_INVITE_MEMBER";
        case CMSG_GUILD_INVITE_RESPONSE:   return "CMSG_GUILD_INVITE_RESPONSE";
        case CMSG_GUILD_PROMOTE_MEMBER:    return "CMSG_GUILD_PROMOTE_MEMBER";
        case CMSG_GUILD_DEMOTE_MEMBER:     return "CMSG_GUILD_DEMOTE_MEMBER";
        case CMSG_GUILD_MOTD:              return "CMSG_GUILD_MOTD";
        case CMSG_UPDATE_ARENA_STATUS:     return "CMSG_UPDATE_ARENA_STATUS";
        case CMSG_SET_IGNORE_PLAYER:       return "CMSG_SET_IGNORE_PLAYER";
        case CMSG_REPORT_PLAYER:           return "CMSG_REPORT_PLAYER";
        case CMSG_MOD_BAN_PLAYER:          return "CMSG_MOD_BAN_PLAYER";
        case CMSG_MOD_KICK_PLAYER:         return "CMSG_MOD_KICK_PLAYER";
        case CMSG_MOD_MUTE_PLAYER:         return "CMSG_MOD_MUTE_PLAYER";
        case CMSG_MOD_WARN_PLAYER:         return "CMSG_MOD_WARN_PLAYER";
        case CMSG_LOGIN:                   return "CMSG_LOGIN";

        default:                           return "UNKNOWN_OPCODE";
    }
}

// Check if an opcode is within the valid range accepted by the client
inline bool isValidOpcode(uint16 opcode) {
    return opcode >= 1 && opcode <= MAX_OPCODE;
}
