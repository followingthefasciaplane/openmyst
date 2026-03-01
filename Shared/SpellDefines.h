#pragma once

#include <cstdint>

namespace SpellDefines
{
    // Combat hit result codes (from Server_CombatMsg handler at 0x00468110)
    // switch on hitResult byte: 0=default(normal), 1="Hit", 2="Resist", 3="Evade",
    // 4="Dodge", 5="Block", 6="Parry", 7=SpellVisual, 8="Immune", 9="Absorb"
    enum class HitResult : int
    {
        Normal       = 0,
        Hit          = 1,
        Resist       = 2,
        Evade        = 3,
        Dodge        = 4,
        Block        = 5,
        Parry        = 6,
        SpellVisual  = 7,   // Animation only, no text (spell template 0x53)
        Immune       = 8,
        Absorb       = 9,
    };

    // Spell effect types
    enum class EffectType : int
    {
        None             = 0,
        SchoolDamage     = 1,
        WeaponDamage     = 2,
        Heal             = 3,
        PeriodicDamage   = 4,
        PeriodicHeal     = 5,
        PeriodicMeleeDamage = 6,
        ApplyAura        = 7,
        HealthDrain      = 8,
        TakeDamage       = 9,
        TakeDamage_Direct = 10,
        AbsorbDamage     = 11,
    };

    // Spell target types
    enum class TargetType : int
    {
        Self         = 0,
        SingleEnemy  = 1,
        SingleFriend = 2,
        AoeEnemy     = 3,
        AoeFriend    = 4,
        Terrain      = 5,
    };

    // Spell condition modifiers
    enum class SpellCondition : int
    {
        None                = 0,
        ImpossibleMiss      = 1,
        ImpossibleDodge     = 2,
        IgnoreArmor         = 3,
        PersistsThroughDeath = 4,
    };

    // Spell cost types
    enum class CostType : int
    {
        None       = 0,
        Mana       = 1,
        Health     = 2,
        HealthPct  = 3,
    };
}
