#pragma once

#include <cstdint>
#include "../Geo2d.h"

// Object-level defines shared between client and server
namespace ObjDefines
{
    // Object variable IDs - stored in MutualObject::m_variables vector.
    // Set by server via ObjectVariable packets (opcode 0x52).
    // Variable ID is uint8_t on the wire (max 255 variables).
    // Confirmed via decompilation of handler at 0x00466030.
    //
    // Indices 0-34 verified from binary switch tables and packet handlers.
    // Dynamic/gossip variables (indices TBD) need further binary analysis.
    enum Variable : int
    {
        None             = 0,
        Health           = 1,
        MaxHealth        = 2,
        Mana             = 3,
        MaxMana          = 4,
        Level            = 5,
        Experience       = 6,
        Gold             = 7,
        Strength         = 8,
        Agility          = 9,
        Intelligence     = 10,
        Stamina          = 11,
        Armor            = 12,
        AttackPower      = 13,
        SpellPower       = 14,
        CritChance       = 15,
        DodgeChance      = 16,
        BlockChance      = 17,
        HitChance        = 18,
        AttackSpeed      = 19,
        MoveSpeed        = 20,
        StatPoints       = 21,
        SpellPoints      = 22,
        PvpFlag          = 23,
        DeathState       = 24,
        Name             = 25,
        Subname          = 26,
        ToggleState      = 27,
        Orientation      = 28,
        Flags            = 29,
        MaxExperience    = 30,
        ArenaRating      = 31,
        GuildId          = 32,
        GuildRank        = 33,
        PkCount          = 34,

        // StatsStart marks the beginning of a block of stat variable IDs.
        // Used by Equipment.cpp: UnitDefines::Stat(var - StatsStart)
        // TODO: verify binary index (likely 35 or higher)
        StatsStart       = 35,
        StatsEnd         = 99,   // TODO: verify binary range

        // Dynamic variables — indices need binary verification via
        // decompilation of ClientUnit::notifyVariableChange and
        // ClientGameObj::notifyVariableChange switch tables.
        // TODO: Decompile and assign correct indices.
        ModelId          = 100,
        ModelScale       = 101,
        MechanicMask     = 102,
        IsAnimating      = 103,
        IsSliding        = 104,
        MoveSpeedPct     = 105,
        GoFlags          = 106,
        DynInteractable  = 107,
        DynLootable      = 108,
        DynSparkle       = 109,
        DynGossipStatus  = 110,
        DynGreyTagged    = 111,
        LockpickingLevel = 112,
        Money            = 113,
        Progression      = 114,
        NumInvestedSpells = 115,
        InArenaQueue     = 116,
        Elite            = 117,
        Boss             = 118,
        InCombat         = 119,
        CombatRating     = 120,
        CombatRatingMax  = 121,
        MailLoot         = 122,
        GameMaster       = 123,
        GmInvisible      = 124,
        ZoneType         = 125,
        FactionId        = 126,
        Moderator        = 127,
        ReactionMask_Out = 128,
        ReactionMask_In  = 129,
        PvP              = 130,  // distinct from PvpFlag?
    };

    // Gossip status values (used with DynGossipStatus variable)
    constexpr int GossipNone = 0;

    namespace GossipStatus
    {
        constexpr int None             = 0;
        constexpr int GossipAvailable  = 1;
        constexpr int QuestAvailable   = 2;
        constexpr int QuestComplete    = 3;
    }

    // Object types in the world (from MutualObject constructors)
    // ClientGameObj: param_1[0x12] = 1, ClientPlayer: param_1[0x12] = 2, ClientNpc: param_1[0x12] = 3
    enum ObjectType : int
    {
        Type_None       = 0,
        Type_GameObject = 1,
        Type_Player     = 2,
        Type_Npc        = 3,
    };
}
