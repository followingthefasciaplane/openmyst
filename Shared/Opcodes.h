#pragma once

#include <cstdint>

// Opcode definitions for Dreadmyst packet protocol.
// Verified against Dreadmyst.exe r1189.
//
// Server -> Client: processPacket switch at 0x00459b80 (74 cases)
// Client -> Server: traced via xrefs to WriteUint16 at 0x004157a0 (61 unique)
//
// Some opcodes in the 0x4B+ range are bidirectional (same opcode, both directions).
// Client-only opcodes occupy the 0x04-0x4A range.

namespace Opcode
{
    // ========================================================================
    //  Mutual packets (bidirectional)
    // ========================================================================
    constexpr uint16_t Mutual_Ping                  = 0x01;

    // ========================================================================
    //  Client -> Server packets (0x04-0x4A range, client-only)
    //  Discovered by tracing callers of WriteUint16 (FUN_004157a0)
    // ========================================================================
    constexpr uint16_t Client_Authenticate          = 0x04;  // FUN_00419160 - login credentials
    constexpr uint16_t Client_EnterWorld            = 0x05;  // FUN_0041f4e0 - select character (world type=4)
    constexpr uint16_t Client_EquipItem             = 0x09;  // FUN_00447d40 - equip from inventory
    constexpr uint16_t Client_EquipItemBank         = 0x0A;  // FUN_0048bbe0 - equip from bank
    constexpr uint16_t Client_UseSpell              = 0x0B;  // FUN_00448060 - use item/spell from slot
    constexpr uint16_t Client_CastSpellTarget       = 0x0C;  // FUN_0048bbe0 - cast on NPC target
    constexpr uint16_t Client_ChatMsg               = 0x11;  // FUN_0046e330 - send chat message
    constexpr uint16_t Client_GuildMotd             = 0x12;  // FUN_00486d80/FUN_00487340 - set guild MOTD
    constexpr uint16_t Client_GuildKickMember       = 0x14;  // FUN_004737a0 - /gkick command
    constexpr uint16_t Client_GuildYieldLeader      = 0x15;  // FUN_004737a0 - /yield command
    constexpr uint16_t Client_GuildInviteMember     = 0x16;  // FUN_0046e0a0/FUN_004737a0 - /ginvite
    constexpr uint16_t Client_GuildCreate           = 0x17;  // FUN_0046e060/FUN_004737a0 - /gcreate
    constexpr uint16_t Client_QuestInteract         = 0x18;  // FUN_004589c0 - interact with quest giver
    constexpr uint16_t Client_QuestAccept           = 0x19;  // FUN_00486d50 - accept quest
    constexpr uint16_t Client_QuestReward           = 0x1A;  // FUN_00486d00 - claim quest reward
    constexpr uint16_t Client_CastSpell             = 0x1B;  // FUN_004218d0 - cast spell
    constexpr uint16_t Client_GuildLeave            = 0x1C;  // FUN_00489200/FUN_00464e00 - leave guild
    constexpr uint16_t Client_SetTarget             = 0x1D;  // FUN_0053c5c0 - select target
    constexpr uint16_t Client_SetIgnorePlayer       = 0x1F;  // FUN_0046e0e0 - add/remove ignore
    constexpr uint16_t Client_InteractObject        = 0x20;  // FUN_004171e0 - interact with game object
    constexpr uint16_t Client_Sit                   = 0x25;  // FUN_00458c20 - sit/stand toggle
    constexpr uint16_t Client_ActionBar             = 0x28;  // FUN_005308a0 - save action bar layout
    constexpr uint16_t Client_ResizeUI              = 0x29;  // FUN_00519eb0 - notify UI resize
    constexpr uint16_t Client_DeselectTarget        = 0x2A;  // FUN_00449df0/FUN_0044aa10 - deselect
    constexpr uint16_t Client_PartyInviteMember     = 0x2F;  // FUN_0046e680/FUN_004737a0 - /invite
    constexpr uint16_t Client_GuildQuit             = 0x32;  // FUN_004737a0 - /gquit
    constexpr uint16_t Client_GuildDisband          = 0x33;  // FUN_004737a0 - /gdisband
    constexpr uint16_t Client_LootItem              = 0x36;  // FUN_0048bbe0 - loot item from corpse
    constexpr uint16_t Client_UnBankItem            = 0x3B;  // FUN_00415a70 - withdraw from bank
    constexpr uint16_t Client_CloseBank             = 0x3C;  // FUN_004161d0 - close bank window
    constexpr uint16_t Client_MoveInventory         = 0x3D;  // FUN_00416490 - swap inventory slots
    constexpr uint16_t Client_MoveInventoryToBank   = 0x3E;  // FUN_00415b80 - deposit to bank
    constexpr uint16_t Client_EnterWorldAlt         = 0x41;  // FUN_0041f4e0 - enter world (type=0x17)
    constexpr uint16_t Client_Emote                 = 0x43;  // FUN_00458ab0 - play emote
    constexpr uint16_t Client_Roll                  = 0x44;  // FUN_004737a0 - /roll
    constexpr uint16_t Client_AFK                   = 0x45;  // FUN_004737a0 - /afk toggle
    constexpr uint16_t Client_ChannelJoin           = 0x46;  // FUN_0046e270 - join chat channel
    constexpr uint16_t Client_ChannelLeave          = 0x47;  // FUN_0046e2b0 - leave chat channel
    constexpr uint16_t Client_ChannelList           = 0x48;  // FUN_0046e2f0 - request channel users
    constexpr uint16_t Client_FriendAdd             = 0x49;  // FUN_0046e230 - add friend
    constexpr uint16_t Client_FriendRemove          = 0x4A;  // FUN_0046e1f0 - remove friend

    // ========================================================================
    //  Client -> Server packets (0x4B+ range, bidirectional)
    //  Same opcode used for both C->S request and S->C response.
    //  The Client_ alias references the Server_ constant.
    // ========================================================================
    constexpr uint16_t Client_VendorBuy             = 0x4B;  // FUN_004547a0 (S->C: CharacterList)
    constexpr uint16_t Client_InspectPlayer         = 0x53;  // FUN_004545c0 (S->C: InspectReveal)
    constexpr uint16_t Client_Inventory             = 0x55;  // FUN_00456470 (S->C: Inventory)
    constexpr uint16_t Client_AcceptDuel            = 0x58;  // FUN_004541b0 (S->C: OfferDuel)
    constexpr uint16_t Client_UnitSpline            = 0x5A;  // FUN_00452e50 (S->C: UnitSpline)
    constexpr uint16_t Client_ChatMsgFull           = 0x5B;  // FUN_00452b00 (S->C: ChatMsg)
    constexpr uint16_t Client_GuildRosterRequest    = 0x5E;  // FUN_00452ab0 (S->C: GuildAddMember)
    constexpr uint16_t Client_GuildRoster           = 0x5F;  // FUN_00452400 (S->C: GuildRoster)
    constexpr uint16_t Client_SetSubname            = 0x60;  // FUN_00452120 (S->C: SetSubname)
    constexpr uint16_t Client_GuildInviteResponse   = 0x61;  // FUN_004520a0 (S->C: GuildInvite)
    constexpr uint16_t Client_SpellGo               = 0x64;  // FUN_00457330 (S->C: SpellGo)
    constexpr uint16_t Client_NotifyItemAdd         = 0x67;  // FUN_004577e0 (S->C: NotifyItemAdd)
    constexpr uint16_t Client_OnObjectWasLooted     = 0x68;  // FUN_00457b50 (S->C: OnObjectWasLooted)
    constexpr uint16_t Client_UnitAuras             = 0x6C;  // FUN_00455990 (S->C: UnitAuras)
    constexpr uint16_t Client_SpellbookUpdate       = 0x6D;  // FUN_00456ef0 (S->C: Spellbook_Update)
    constexpr uint16_t Client_GossipMenu            = 0x6E;  // FUN_00457e50 (S->C: GossipMenu)
    constexpr uint16_t Client_AvailableWorldQuests  = 0x6F;  // FUN_004567a0 (S->C: AvailableWorldQuests)
    constexpr uint16_t Client_QueryWaypoints        = 0x78;  // FUN_00456650 (S->C: QueryWaypointsResponse)
    constexpr uint16_t Client_Spellbook             = 0x7A;  // FUN_00456c40 (S->C: Spellbook)
    constexpr uint16_t Client_QuestListRequest      = 0x82;  // FUN_00458620 (S->C: QuestList)
    constexpr uint16_t Client_AcceptParty           = 0x84;  // FUN_004541f0 (S->C: OfferParty)
    constexpr uint16_t Client_PartyList             = 0x85;  // FUN_00454230 (S->C: PartyList)
    constexpr uint16_t Client_MarkNpcs              = 0x87;  // FUN_004528e0 (S->C: MarkNpcsOnMap)
    constexpr uint16_t Client_TradeUpdate           = 0x89;  // FUN_00453380 (S->C: TradeUpdate)
    constexpr uint16_t Client_GuildNotifyRoleChange = 0x8B;  // FUN_004521d0 (S->C: GuildNotifyRoleChange)
    constexpr uint16_t Client_GuildOnlineStatus     = 0x8C;  // FUN_004522f0 (S->C: GuildOnlineStatus)
    constexpr uint16_t Client_Bank                  = 0x8D;  // FUN_00455fd0 (S->C: Bank)
    constexpr uint16_t Client_ChannelInfo           = 0x8F;  // FUN_00454b10 (S->C: ChannelInfo)
    constexpr uint16_t Client_PkNotify              = 0x96;  // FUN_00452170 (S->C: PkNotify)

    // ========================================================================
    //  Server -> Client packets (0x03-0x97 range)
    //  Verified against processPacket switch at 0x00459b80 (74 cases)
    // ========================================================================
    constexpr uint16_t Server_Validate              = 0x03;
    constexpr uint16_t Server_CharacterList          = 0x4B;
    constexpr uint16_t Server_NewWorld               = 0x4C;
    constexpr uint16_t Server_Player                 = 0x4D;
    constexpr uint16_t Server_Npc                    = 0x4E;
    constexpr uint16_t Server_GameObject             = 0x4F;
    constexpr uint16_t Server_DestroyObject          = 0x50;
    constexpr uint16_t Server_CharaCreateResult       = 0x51;
    constexpr uint16_t Server_ObjectVariable         = 0x52;
    constexpr uint16_t Server_InspectReveal          = 0x53;
    constexpr uint16_t Server_AggroMob               = 0x54;
    constexpr uint16_t Server_Inventory              = 0x55;
    constexpr uint16_t Server_EquipItem              = 0x56;
    constexpr uint16_t Server_ChatError              = 0x57;
    constexpr uint16_t Server_OfferDuel              = 0x58;
    constexpr uint16_t Server_WorldError             = 0x59;
    constexpr uint16_t Server_UnitSpline             = 0x5A;
    constexpr uint16_t Server_ChatMsg                = 0x5B;
    // 0x5C-0x5D gap
    constexpr uint16_t Server_GuildAddMember         = 0x5E;
    constexpr uint16_t Server_GuildRoster            = 0x5F;
    constexpr uint16_t Server_SetSubname             = 0x60;
    constexpr uint16_t Server_GuildInvite            = 0x61;
    constexpr uint16_t Server_CastStart              = 0x62;
    constexpr uint16_t Server_CastStop               = 0x63;
    constexpr uint16_t Server_SpellGo                = 0x64;
    constexpr uint16_t Server_UnitTeleport           = 0x65;
    constexpr uint16_t Server_CombatMsg              = 0x66;
    constexpr uint16_t Server_NotifyItemAdd          = 0x67;
    constexpr uint16_t Server_OnObjectWasLooted      = 0x68;
    constexpr uint16_t Server_UnitOrientation        = 0x69;
    constexpr uint16_t Server_RespawnResponse        = 0x6A;
    constexpr uint16_t Server_Cooldown               = 0x6B;
    constexpr uint16_t Server_UnitAuras              = 0x6C;
    constexpr uint16_t Server_Spellbook_Update       = 0x6D;
    constexpr uint16_t Server_GossipMenu             = 0x6E;
    constexpr uint16_t Server_AvailableWorldQuests   = 0x6F;
    constexpr uint16_t Server_AcceptedQuest          = 0x70;
    constexpr uint16_t Server_QuestTally             = 0x71;
    constexpr uint16_t Server_QuestComplete          = 0x72;
    constexpr uint16_t Server_RewardedQuest          = 0x73;
    // 0x74 gap
    constexpr uint16_t Server_AbandonQuest           = 0x75;
    constexpr uint16_t Server_SpentGold              = 0x76;
    constexpr uint16_t Server_ExpNotify              = 0x77;
    constexpr uint16_t Server_QueryWaypointsResponse = 0x78;
    constexpr uint16_t Server_SocketResult           = 0x79;
    constexpr uint16_t Server_Spellbook              = 0x7A;
    constexpr uint16_t Server_EmpowerResult          = 0x7B;
    constexpr uint16_t Server_LvlResponse            = 0x7C;
    constexpr uint16_t Server_SetController          = 0x7D;
    constexpr uint16_t Server_LearnedSpell           = 0x7E;
    constexpr uint16_t Server_UpdateVendorStock      = 0x7F;
    constexpr uint16_t Server_RepairCost             = 0x80;
    // 0x81 gap
    constexpr uint16_t Server_QuestList              = 0x82;
    constexpr uint16_t Server_DiscoverWaypointNotify = 0x83;
    constexpr uint16_t Server_OfferParty             = 0x84;
    constexpr uint16_t Server_PartyList              = 0x85;
    constexpr uint16_t Server_OpenLootWindow         = 0x86;
    constexpr uint16_t Server_MarkNpcsOnMap          = 0x87;
    // 0x88 gap
    constexpr uint16_t Server_TradeUpdate            = 0x89;
    constexpr uint16_t Server_TradeCanceled          = 0x8A;
    constexpr uint16_t Server_GuildNotifyRoleChange  = 0x8B;
    constexpr uint16_t Server_GuildOnlineStatus      = 0x8C;
    constexpr uint16_t Server_Bank                   = 0x8D;
    constexpr uint16_t Server_OpenBank               = 0x8E;
    constexpr uint16_t Server_ChannelInfo            = 0x8F;
    constexpr uint16_t Server_PromptRespec           = 0x90;
    constexpr uint16_t Server_ChannelChangeConfirm   = 0x91;
    constexpr uint16_t Server_ArenaReady             = 0x92;
    constexpr uint16_t Server_ArenaQueued            = 0x93;
    constexpr uint16_t Server_ArenaOutcome           = 0x94;
    constexpr uint16_t Server_ArenaStatus            = 0x95;
    constexpr uint16_t Server_PkNotify               = 0x96;
    constexpr uint16_t Server_QueuePosition          = 0x97;

    constexpr uint16_t MAX_OPCODE = 0x98;
}
