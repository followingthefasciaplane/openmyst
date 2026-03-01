#pragma once

#include "GameDefines.h"
#include "PlayerDefines.h"
#include "UnitDefines.h"
#include "ItemDefines.h"
#include "SpellDefines.h"
#include "ChatDefines.h"
#include "GuildDefines.h"
#include "QuestDefines.h"
#include "StlBuffer.h"
#include "..\Geo2d.h"
#include <string>
#include <vector>
#include <cstdint>

// ============================================================================
//  Server -> Client packet structures
//  Binary-verified from Dreadmyst.exe r1189 processPacket at 0x00459b80
//  RTTI strings at 0x67f878+, handler decompilation
//
//  Convention: All fields use m_ prefix. unpack() reads from StlBuffer.
//  Opcodes reference Opcode:: constants from Opcodes.h.
// ============================================================================

// Base class for all server game packets
struct GamePacket
{
    virtual ~GamePacket() = default;
    virtual void unpack(StlBuffer& buf) {}
};

// Shared item slot structure (14 bytes / 0x0E per slot)
// Used by Inventory, Bank, EquipItem, and Player equipment list
// From FUN_004561b0 and FUN_004543a0
struct ItemSlotData
{
    uint16_t m_itemId     = 0;
    uint16_t m_count      = 0;
    uint8_t  m_field3     = 0;
    uint8_t  m_field4     = 0;
    uint8_t  m_field5     = 0;
    uint8_t  m_field6     = 0;
    uint8_t  m_field7     = 0;
    uint8_t  m_field8     = 0;
    bool     m_isEquipped = false;
    uint8_t  m_field10    = 0;
    uint8_t  m_field11    = 0;
};

// --- Opcode 0x03: Server_Validate (handler at 0x0045a940) ---
// Auth result + session token
// switch: 0=success("Loading"), 1="Failed to authenticate", 2="Server full",
//         3="Account suspended", 4="Game needs to be updated"
struct GP_Server_Validate : public GamePacket
{
    int32_t  m_authResult   = 0;    // AccountDefines::AuthenticateResult
    uint64_t m_sessionToken = 0;
};

// --- Opcode 0x4B: Server_CharacterList (handler at 0x0045aec0) ---
// RTTI: CharacterSelection
struct GP_Server_CharacterList : public GamePacket
{
    struct CharEntry
    {
        int32_t     m_guid     = 0;
        std::string m_name;
        int32_t     m_level    = 0;
        int32_t     m_mapId    = 0;
        int32_t     m_classId  = 0;
        int32_t     m_gender   = 0;
        int32_t     m_portrait = 0;
    };
    std::vector<CharEntry> m_characters;
};

// --- Opcode 0x4C: Server_NewWorld (handler at 0x0045b730) ---
// World creation (0x240 bytes), "Welcome to Dreadmyst!"
struct GP_Server_NewWorld : public GamePacket
{
    int32_t m_mapId = 0;
};

// --- Opcode 0x4D: Server_Player (handler at 0x0045bb50) ---
// RTTI: GP_Server_Player::vftable, ClientPlayer(0x1E0)
// Base: FUN_00455010 (model, pos, attributes) + FUN_00455530 (player extension)
struct GP_Server_Player : public GamePacket
{
    // Base object fields (FUN_00455010)
    std::string m_model;
    int32_t     m_posX           = 0;
    int32_t     m_posY           = 0;

    struct Attribute
    {
        uint8_t m_index = 0;    // ObjDefines::Variable
        int32_t m_value = 0;
    };
    std::vector<Attribute> m_attributes;

    // Player extension (FUN_00455530)
    int32_t     m_templateId     = 0;   // offset 0x34
    std::string m_name;                  // offset 0x3C
    std::string m_guildName;             // offset 0x54
    uint8_t     m_gender         = 0;   // offset 0x38
    uint8_t     m_classId        = 0;   // offset 0x39
    uint8_t     m_field3A        = 0;   // offset 0x3A

    // Equipment list
    struct EquipEntry
    {
        uint8_t  m_slotId   = 0;
        uint16_t m_itemId   = 0;
        uint16_t m_field3   = 0;
        uint8_t  m_field4   = 0;
        uint8_t  m_field5   = 0;
        uint8_t  m_field6   = 0;
        uint8_t  m_field7   = 0;
        uint8_t  m_field8   = 0;
        uint8_t  m_field9   = 0;
        bool     m_field10  = false;
        uint8_t  m_field11  = 0;
    };
    std::vector<EquipEntry> m_equipment;
};

// --- Opcode 0x4E: Server_Npc (handler at 0x0045d170) ---
// RTTI: ClientNpc(0x1B8). Base FUN_00455010 + NPC extension FUN_00455280
// Name/subname from npc_template content DB lookup
struct GP_Server_Npc : public GamePacket
{
    // Base object fields
    std::string m_model;
    int32_t     m_posX       = 0;
    int32_t     m_posY       = 0;
    std::vector<GP_Server_Player::Attribute> m_attributes;

    // NPC extension
    int32_t     m_templateId = 0;   // npc_template ID, offset 0x184
};

// --- Opcode 0x4F: Server_GameObject (handler at 0x0045d430) ---
// RTTI: ClientGameObj(0xEC). Base FUN_00455010 only.
// Name from gameobject_template content DB lookup
struct GP_Server_GameObject : public GamePacket
{
    std::string m_model;
    int32_t     m_posX       = 0;
    int32_t     m_posY       = 0;
    std::vector<GP_Server_Player::Attribute> m_attributes;
};

// --- Opcode 0x50: Server_DestroyObject (inline) ---
// Reads guid, calls FUN_00537760
struct GP_Server_DestroyObject : public GamePacket
{
    int32_t m_guid = 0;
};

// --- Opcode 0x51: Server_CharaCreateResult (inline) ---
// CharacterCreation::RTTI
struct GP_Server_CharaCreateResult : public GamePacket
{
    int32_t m_result = 0;
};

// --- Opcode 0x52: Server_ObjectVariable (handler at 0x00466030) ---
// guid + varId(byte) + value
struct GP_Server_ObjectVariable : public GamePacket
{
    int32_t m_guid   = 0;
    uint8_t m_varId  = 0;    // ObjDefines::Variable
    int32_t m_value  = 0;
};

// --- Opcode 0x53: Server_InspectReveal (inline) ---
// GP_Server_InspectReveal::vftable
struct GP_Server_InspectReveal : public GamePacket
{
    int32_t     m_targetGuid = 0;
    std::string m_name;
    int32_t     m_classId    = 0;
    int32_t     m_level      = 0;
    int32_t     m_guildId    = 0;
    std::string m_guildName;
};

// --- Opcode 0x54: Server_AggroMob (inline) ---
// Reads guid, calls FUN_0053c0e0
struct GP_Server_AggroMob : public GamePacket
{
    int32_t m_guid = 0;
};

// --- Opcode 0x55: Server_Inventory (handler at 0x00463fb0) ---
// RTTI: Inventory, Toolbar. Uses shared FUN_004561b0 (14-byte slots)
struct GP_Server_Inventory : public GamePacket
{
    std::vector<ItemSlotData> m_slots;  // count read as uint8, 14 bytes per slot
};

// --- Opcode 0x56: Server_EquipItem (handler at 0x004643f0) ---
// RTTI: ClientPlayer, Equipment. Read via FUN_004543a0
struct GP_Server_EquipItem : public GamePacket
{
    int32_t  m_entityId   = 0;    // target entity
    uint8_t  m_slotIndex  = 0;
    uint16_t m_itemId     = 0;
    uint16_t m_field3     = 0;
    uint8_t  m_field4     = 0;
    uint8_t  m_field5     = 0;
    uint8_t  m_field6     = 0;
    uint8_t  m_field7     = 0;
    uint8_t  m_field8     = 0;
    uint8_t  m_field9     = 0;
    bool     m_isEquipped = false;
    uint8_t  m_field10    = 0;
    bool     m_field11    = false;
};

// --- Opcode 0x57: Server_ChatError (inline) ---
// Reads code(short), calls FUN_0053f530
struct GP_Server_ChatError : public GamePacket
{
    int16_t m_errorCode = 0;
};

// --- Opcode 0x58: Server_OfferDuel (handler at 0x00465dd0) ---
// RTTI: GP_Server_OfferDuel::vftable. "%s wants to duel. Accept?"
struct GP_Server_OfferDuel : public GamePacket
{
    std::string m_challengerName;
};

// --- Opcode 0x59: Server_WorldError (inline) ---
// Reads code(short), calls FUN_0053e6f0. "not in guild" xref
struct GP_Server_WorldError : public GamePacket
{
    int16_t m_errorCode = 0;
};

// --- Opcode 0x5A: Server_UnitSpline (handler at 0x004661b0) ---
// Spline movement path with SQRT distance calc
struct GP_Server_UnitSpline : public GamePacket
{
    int32_t m_guid   = 0;
    float   m_startX = 0.0f;
    float   m_startY = 0.0f;
    std::vector<Geo2d::Vector2> m_spline;
};

// --- Opcode 0x5B: Server_ChatMsg (handler at 0x0045ccb0) ---
// RTTI: GP_Server_ChatMsg::vftable, GameChat. Read via FUN_00452c40
struct GP_Server_ChatMsg : public GamePacket
{
    int32_t     m_senderObjectId = 0;
    uint8_t     m_chatType       = 0;   // ChatDefines::Channels
    std::string m_senderName;
    std::string m_message;
    int16_t     m_channelId1     = 0;
    int16_t     m_channelId2     = 0;
    uint8_t     m_field40        = 0;
    uint8_t     m_field41        = 0;
    uint8_t     m_field42        = 0;
    uint8_t     m_field43        = 0;
    uint8_t     m_field44        = 0;
    uint8_t     m_field45        = 0;
    bool        m_isGM           = false;
    uint8_t     m_field47        = 0;
    uint8_t     m_field48        = 0;
};

// --- Opcode 0x5E: Server_GuildAddMember (handler at 0x0046a960) ---
// RTTI: GP_Server_GuildAddMember::vftable. " has joined the guild."
struct GP_Server_GuildAddMember : public GamePacket
{
    int32_t     m_guildId    = 0;
    std::string m_memberName;
};

// --- Opcode 0x5F: Server_GuildRoster (handler at 0x00466940) ---
// RTTI: GP_Server_GuildRoster::vftable, GuildRoster, TextLines
// Read via FUN_00452690. Member entries are 0x24 (36) bytes each.
struct GP_Server_GuildRoster : public GamePacket
{
    std::string m_guildName;
    std::string m_motd;
    std::vector<GuildDefines::MemberInfo> m_members;  // count as uint16
};

// --- Opcode 0x60: Server_SetSubname (handler at 0x00466760) ---
// RTTI: GP_Server_SetSubname::vftable
struct GP_Server_SetSubname : public GamePacket
{
    int32_t     m_targetId = 0;
    std::string m_subname;
};

// --- Opcode 0x61: Server_GuildInvite (handler at 0x00466e80) ---
// RTTI: GP_Server_GuildInvite::vftable. "You have been invited to join the guild "
struct GP_Server_GuildInvite : public GamePacket
{
    int32_t     m_guildId = 0;
    std::string m_guildName;
};

// --- Opcode 0x62: Server_CastStart (handler at 0x00467c80) ---
// guid + spellId, "unit_cast_animation"
struct GP_Server_CastStart : public GamePacket
{
    int32_t m_guid    = 0;
    int32_t m_spellId = 0;
};

// --- Opcode 0x63: Server_CastStop (handler at 0x00467b00) ---
// guid + spellId, stops animation
struct GP_Server_CastStop : public GamePacket
{
    int32_t m_guid    = 0;
    int32_t m_spellId = 0;
};

// --- Opcode 0x64: Server_SpellGo (handler at 0x0046a130) ---
// RTTI: GP_Server_SpellGo::vftable. Read via FUN_00457520
// Variable-length target/effect list, then ground position
struct GP_Server_SpellGo : public GamePacket
{
    int32_t m_casterObjectId = 0;
    int32_t m_targetObjectId = 0;

    struct SpellEffect
    {
        int32_t m_targetId    = 0;
        int32_t m_effectValue = 0;
    };
    std::vector<SpellEffect> m_effects;   // count as uint16

    float m_groundX = 0.0f;
    float m_groundY = 0.0f;
};

// --- Opcode 0x65: Server_UnitTeleport (handler at 0x00469f10) ---
// guid + x + y + orientation (4 fields)
struct GP_Server_UnitTeleport : public GamePacket
{
    int32_t m_guid        = 0;
    float   m_posX        = 0.0f;
    float   m_posY        = 0.0f;
    float   m_orientation = 0.0f;
};

// --- Opcode 0x66: Server_CombatMsg (handler at 0x00468110) ---
// Read via FUN_00457680. "Parry", "Dodge", "Evade", "Immune", "Absorb", etc.
struct GP_Server_CombatMsg : public GamePacket
{
    int32_t  m_attackerObjectId = 0;
    int32_t  m_targetObjectId  = 0;
    uint8_t  m_hitResult       = 0;    // SpellDefines::HitResult
    int16_t  m_damage          = 0;
    int16_t  m_school          = 0;    // spell school/element
    uint8_t  m_field6          = 0;
    bool     m_isCrit          = false;
    uint8_t  m_field8          = 0;
    bool     m_isPet           = false;
};

// --- Opcode 0x67: Server_NotifyItemAdd (handler at 0x004673c0) ---
// Read via FUN_004578a0. "item_template", "You receive: ["
struct GP_Server_NotifyItemAdd : public GamePacket
{
    uint16_t m_itemId  = 0;
    uint16_t m_count   = 0;
    uint8_t  m_field3  = 0;
    uint8_t  m_field4  = 0;
    uint8_t  m_field5  = 0;
    uint8_t  m_field6  = 0;
    uint8_t  m_field7  = 0;
    uint8_t  m_field8  = 0;
    bool     m_field9  = false;
};

// --- Opcode 0x68: Server_OnObjectWasLooted (handler at 0x00467130) ---
// LootWindow RTTI, 2 guids
struct GP_Server_OnObjectWasLooted : public GamePacket
{
    int32_t m_objectGuid = 0;
    int32_t m_looterGuid = 0;
};

// --- Opcode 0x69: Server_UnitOrientation (handler at 0x00469d50) ---
// guid + float(orientation)
struct GP_Server_UnitOrientation : public GamePacket
{
    int32_t m_guid        = 0;
    float   m_orientation = 0.0f;
};

// --- Opcode 0x6A: Server_RespawnResponse (handler at 0x0045c070) ---
// bool result, "teleport.wav" on success
struct GP_Server_RespawnResponse : public GamePacket
{
    bool m_success = false;
};

// --- Opcode 0x6B: Server_Cooldown (inline) ---
// _Xtime_get_ticks timing
struct GP_Server_Cooldown : public GamePacket
{
    int32_t m_spellId  = 0;
    int32_t m_duration = 0;
};

// --- Opcode 0x6C: Server_UnitAuras (handler at 0x00469650) ---
// RTTI: GP_Server_UnitAuras::vftable. Read via FUN_00455d20
// Wire read order per entry: u16 spellId, i32 elapsedTime, i32 maxDuration, u8 stackCount, i32 casterGuid
// Struct size: 0x20 (32 bytes) per entry, stride confirmed from binary loop
struct GP_Server_UnitAuras : public GamePacket
{
    struct AuraEntry
    {
        uint16_t m_spellId     = 0;    // +0x00: spell/aura identifier (wire: u16)
        uint8_t  m_stackCount  = 0;    // +0x02: stack count (wire: u8)
        // +0x03: padding
        int32_t  m_casterGuid  = 0;    // +0x04: caster unit guid (wire: i32)
        int32_t  m_elapsedTime = 0;    // +0x08: ms elapsed since applied (wire: i32)
        int32_t  m_maxDuration = 0;    // +0x0C: total duration in ms (wire: i32)
        int64_t  m_startDate   = 0;    // +0x10: client-computed (timeNow - elapsed)
        int64_t  m_endDate     = 0;    // +0x18: client-computed (timeNow + maxDuration - elapsed)
    };

    int32_t m_unitGuid = 0;
    std::vector<AuraEntry> m_buffs;     // count as uint8
    std::vector<AuraEntry> m_debuffs;   // count as uint8
};

// --- Opcode 0x6D: Server_Spellbook_Update (handler at 0x00463850) ---
// Toolbar assignment, spell type IDs
struct GP_Server_Spellbook_Update : public GamePacket
{
    int32_t m_spellId = 0;
    int32_t m_rank    = 0;
    bool    m_learned = true;
};

// --- Opcode 0x6E: Server_GossipMenu (handler at 0x00460fb0) ---
// RTTI: GP_Server_GossipMenu::vftable, DialogNpc, QuestOffer, QuestComplete, VendorNpc
// Read via FUN_004582c0. Complex multi-array structure.
struct GP_Server_GossipMenu : public GamePacket
{
    struct GossipOption
    {
        uint16_t m_icon     = 0;
        uint16_t m_optionId = 0;
        uint8_t  m_coded    = 0;
        uint8_t  m_unknown1 = 0;
        int32_t  m_boxMoney = 0;
        uint8_t  m_hasBoxText = 0;
    };

    int32_t m_npcGuid             = 0;
    int32_t m_questGiverType      = 0;
    std::vector<int32_t> m_questOptionIds;
    std::vector<int32_t> m_questCompleteIds;
    std::vector<int32_t> m_gossipTextIds;
    std::vector<GossipOption> m_gossipOptions;
};

// --- Opcode 0x6F: Server_AvailableWorldQuests (handler at 0x0045d660) ---
// Silent QuestLog update
struct GP_Server_AvailableWorldQuests : public GamePacket
{
    std::vector<int32_t> m_questIds;
};

// --- Opcode 0x70: Server_AcceptedQuest (handler at 0x00460990) ---
// RTTI: QuestLog, GameChat. "Quest accepted: %s", "alert_quest_get.ogg"
struct GP_Server_AcceptedQuest : public GamePacket
{
    int32_t m_questId = 0;
};

// --- Opcode 0x71: Server_QuestTally (handler at 0x00460760) ---
// RTTI: QuestLog. 3 ints + byte (tallyType), 4 update functions
struct GP_Server_QuestTally : public GamePacket
{
    int32_t m_param1    = 0;
    int32_t m_param2    = 0;
    int32_t m_param3    = 0;
    uint8_t m_tallyType = 0;   // QuestDefines::TallyType
};

// --- Opcode 0x72: Server_QuestComplete (handler at 0x0045fb00) ---
// questId + done, "Quest Complete" gold text
struct GP_Server_QuestComplete : public GamePacket
{
    int32_t m_questId    = 0;
    bool    m_isComplete = false;
};

// --- Opcode 0x73: Server_RewardedQuest (handler at 0x0045f600) ---
// "%s completed!", "alert_quest_reward_a.ogg"
struct GP_Server_RewardedQuest : public GamePacket
{
    int32_t m_questId = 0;
};

// --- Opcode 0x75: Server_AbandonQuest (handler at 0x0045f0d0) ---
// "Abandoned quest: %s"
struct GP_Server_AbandonQuest : public GamePacket
{
    int32_t m_questId = 0;
};

// --- Opcode 0x76: Server_SpentGold (handler at 0x0045edd0) ---
// "You spent %d Gold"
struct GP_Server_SpentGold : public GamePacket
{
    int32_t m_amount = 0;
};

// --- Opcode 0x77: Server_ExpNotify (handler at 0x0045e320) ---
// "You gained %d experience points."
struct GP_Server_ExpNotify : public GamePacket
{
    int32_t m_experience = 0;
};

// --- Opcode 0x78: Server_QueryWaypointsResponse (handler at 0x0045d990) ---
// RTTI: MapQuester. Single byte flag, resets waypoint collection
struct GP_Server_QueryWaypointsResponse : public GamePacket
{
    uint8_t m_waypointData = 0;
};

// --- Opcode 0x79: Server_SocketResult (handler at 0x0045c770) ---
// "filled a socket" success/fail
struct GP_Server_SocketResult : public GamePacket
{
    bool m_success = false;
};

// --- Opcode 0x7A: Server_Spellbook (handler at 0x00463600) ---
// Loads spells, toolbar refresh, inventory
struct GP_Server_Spellbook : public GamePacket
{
    struct SpellEntry
    {
        int32_t m_spellId = 0;
        int32_t m_rank    = 0;
    };
    std::vector<SpellEntry> m_spells;
};

// --- Opcode 0x7B: Server_EmpowerResult (handler at 0x0045c240) ---
// "empowered the item" success/fail
struct GP_Server_EmpowerResult : public GamePacket
{
    bool m_success = false;
};

// --- Opcode 0x7C: Server_LvlResponse (handler at 0x00465900) ---
// Equipment/Inventory RTTI
struct GP_Server_LvlResponse : public GamePacket
{
    int32_t m_level = 0;
};

// --- Opcode 0x7D: Server_SetController (handler at 0x00465780) ---
// guid, target update
struct GP_Server_SetController : public GamePacket
{
    int32_t m_guid = 0;
};

// --- Opcode 0x7E: Server_LearnedSpell (handler at 0x00465330) ---
// "You learned a new ability: ", spell_template
struct GP_Server_LearnedSpell : public GamePacket
{
    int32_t m_spellId = 0;
};

// --- Opcode 0x7F: Server_UpdateVendorStock (handler at 0x00463470) ---
// VendorNpc::RTTI
struct GP_Server_UpdateVendorStock : public GamePacket
{
    int32_t m_npcGuid = 0;
};

// --- Opcode 0x80: Server_RepairCost (handler at 0x004630c0) ---
// "Repair costs %sg. Accept?"
struct GP_Server_RepairCost : public GamePacket
{
    int32_t m_cost = 0;
};

// --- Opcode 0x82: Server_QuestList (handler at 0x00461770) ---
// RTTI: MapQuester. "social_critical2_a.ogg"
struct GP_Server_QuestList : public GamePacket
{
    std::vector<int32_t> m_questIds;   // count as uint16
};

// --- Opcode 0x83: Server_DiscoverWaypointNotify (handler at 0x00462e50) ---
// "You've discovered a new waypoint!"
struct GP_Server_DiscoverWaypointNotify : public GamePacket
{
    int32_t m_waypointId = 0;
};

// --- Opcode 0x84: Server_OfferParty (handler at 0x00462be0) ---
// RTTI: GP_Server_OfferParty::vftable. "%s group invited you. Accept?"
struct GP_Server_OfferParty : public GamePacket
{
    std::string m_inviterName;
};

// --- Opcode 0x85: Server_PartyList (handler at 0x00462a50) ---
// leader guid + member vector
struct GP_Server_PartyList : public GamePacket
{
    int32_t m_partyId = 0;
    std::vector<int32_t> m_memberIds;   // count as uint8
};

// --- Opcode 0x86: Server_OpenLootWindow (handler at 0x00461d20) ---
// RTTI: LootWindow
struct GP_Server_OpenLootWindow : public GamePacket
{
    int32_t m_lootSourceId = 0;
    int32_t m_lootCount    = 0;
};

// --- Opcode 0x87: Server_MarkNpcsOnMap (handler at 0x00461ab0) ---
// RTTI: MapQuester, NPC marker trees
struct GP_Server_MarkNpcsOnMap : public GamePacket
{
    struct NpcMarker
    {
        int32_t m_npcGuid = 0;
        float   m_posX    = 0.0f;
        float   m_posY    = 0.0f;
        int32_t m_icon    = 0;
    };
    std::vector<NpcMarker> m_markers;
};

// --- Opcode 0x89: Server_TradeUpdate (handler at 0x00461fa0) ---
// RTTI: GP_Server_TradeUpdate::vftable, TradeWindow, ClientPlayer
// Read via FUN_00453ab0. Complex nested structure.
struct GP_Server_TradeUpdate : public GamePacket
{
    struct TradeItem
    {
        uint16_t m_itemId    = 0;
        uint16_t m_quantity  = 0;
        uint8_t  m_slot      = 0;
        uint8_t  m_type      = 0;
        uint8_t  m_flag1     = 0;
        uint8_t  m_flag2     = 0;
        uint8_t  m_byte8     = 0;
        uint8_t  m_byte9     = 0;
        uint8_t  m_byteA     = 0;
        uint32_t m_extraData = 0;
    };  // 15 bytes per entry

    struct TradeSlot
    {
        int32_t  m_entityId = 0;
        std::vector<TradeItem> m_items;   // count as uint16
    };

    int32_t m_tradeContextId  = 0;
    bool    m_myConfirm       = false;
    bool    m_theirConfirm    = false;
    int32_t m_myGold          = 0;
    int32_t m_theirGold       = 0;
    std::vector<TradeSlot> m_mySlots;      // count as uint16
    std::vector<TradeSlot> m_theirSlots;   // count as uint16
};

// --- Opcode 0x8A: Server_TradeCanceled (handler at 0x004628d0) ---
// TradeWindow close, no payload
struct GP_Server_TradeCanceled : public GamePacket
{
};

// --- Opcode 0x8B: Server_GuildNotifyRoleChange (handler at 0x00464e00) ---
// RTTI: GP_Server_GuildNotifyRoleChange::vftable
// "'s rank in the guild is now " + rank name
struct GP_Server_GuildNotifyRoleChange : public GamePacket
{
    std::string m_memberName;
    uint8_t     m_newRank = 0;   // GuildDefines::Rank
};

// --- Opcode 0x8C: Server_GuildOnlineStatus (handler at 0x004646b0) ---
// RTTI: GP_Server_GuildOnlineStatus::vftable
// " has come online." / " has logged off."
struct GP_Server_GuildOnlineStatus : public GamePacket
{
    std::string m_memberName;
    bool        m_online = false;
};

// --- Opcode 0x8D: Server_Bank (handler at 0x00463bf0) ---
// RTTI: Bank, Toolbar. Uses same FUN_004561b0 as Inventory (14-byte slots)
struct GP_Server_Bank : public GamePacket
{
    std::vector<ItemSlotData> m_slots;   // count as uint8, 14 bytes per slot
};

// --- Opcode 0x8E: Server_OpenBank (inline) ---
// Opens panels 0x16, 0x17, 0x1A, 0x13
struct GP_Server_OpenBank : public GamePacket
{
};

// --- Opcode 0x8F: Server_ChannelInfo (handler at 0x0045b400) ---
// RTTI: GP_Server_ChannelInfo::vftable, Minimap
// Read via FUN_00454bd0. "Channel " prefix
struct GP_Server_ChannelInfo : public GamePacket
{
    int32_t m_field1       = 0;
    int32_t m_field2       = 0;
    std::vector<int32_t> m_memberIds;   // count as int32
};

// --- Opcode 0x90: Server_PromptRespec (handler at 0x00464c30) ---
// "Spend g to respec?"
struct GP_Server_PromptRespec : public GamePacket
{
    int32_t m_cost = 0;
};

// --- Opcode 0x91: Server_ChannelChangeConfirm (handler at 0x00464a20) ---
// "You've changed channels"
struct GP_Server_ChannelChangeConfirm : public GamePacket
{
    int32_t m_channelId = 0;
};

// --- Opcode 0x92: Server_ArenaReady (handler at 0x004605e0) ---
// "Your arena match is ready"
struct GP_Server_ArenaReady : public GamePacket
{
};

// --- Opcode 0x93: Server_ArenaQueued (handler at 0x0045fd90) ---
// bool, queue join sounds
struct GP_Server_ArenaQueued : public GamePacket
{
    bool m_queued = false;
};

// --- Opcode 0x94: Server_ArenaOutcome (handler at 0x00460310) ---
// bool, victory/defeat sounds
struct GP_Server_ArenaOutcome : public GamePacket
{
    bool m_victory = false;
};

// --- Opcode 0x95: Server_ArenaStatus (handler at 0x00460070) ---
// bool, "start_arena.ogg"
struct GP_Server_ArenaStatus : public GamePacket
{
    bool m_started = false;
};

// --- Opcode 0x96: Server_PkNotify (handler at 0x0045de50) ---
// RTTI: GP_Server_PkNotify::vftable. "You have killed "
// Only contains victim name; killer is implied (local player)
struct GP_Server_PkNotify : public GamePacket
{
    std::string m_victimName;
};

// --- Opcode 0x97: Server_QueuePosition (inline) ---
// "Position in queue:"
struct GP_Server_QueuePosition : public GamePacket
{
    int32_t m_position = 0;
};
