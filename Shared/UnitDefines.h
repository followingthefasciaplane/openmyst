#pragma once

#include <cstdint>

namespace UnitDefines
{
    // Unit factions - determines hostility, name color, PvP rules
    enum class Faction : int
    {
        Neutral    = 0,
        Friendly   = 1,
        Hostile    = 2,
        Alliance   = 3,   // Player faction
        PvpFlagged = 4,
    };

    // Body part animation slots
    enum class AnimId : int
    {
        Idle           = 0,
        Walk           = 1,
        Run            = 2,
        Attack         = 3,
        AttackOffhand  = 4,
        CastStart      = 5,
        CastChannel    = 6,
        Death          = 7,
        Damaged        = 8,
        Block          = 9,
        Dodge          = 10,
        Special1       = 11,
        Special2       = 12,
        Sit            = 13,
        Dance          = 14,
        Point          = 15,
        Wave           = 16,
        Bow            = 17,
    };

    // Facing direction (8-directional)
    enum class Direction : int
    {
        South     = 0,
        SouthWest = 1,
        West      = 2,
        NorthWest = 3,
        North     = 4,
        NorthEast = 5,
        East      = 6,
        SouthEast = 7,
    };

    // Equipment slots
    enum EquipSlot : int
    {
        Slot_None       = -1,
        Slot_Head       = 0,
        Slot_Chest      = 1,
        Slot_Legs       = 2,
        Slot_Feet       = 3,
        Slot_Hands      = 4,
        Slot_Weapon     = 5,
        Slot_Offhand    = 6,
        Slot_Ranged     = 7,
        Slot_Ring1      = 8,
        Slot_Ring2      = 9,
        Slot_Amulet     = 10,
        Slot_Max        = 11,
    };

    // Death states
    enum class DeathState : int
    {
        Alive           = 0,
        Dead            = 1,
        Corpse          = 2,
    };
}
