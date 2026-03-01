#pragma once

#include <cstdint>
#include <string>

namespace ItemDefines
{
    // Item quality/rarity levels
    enum class Quality : int
    {
        Poor      = 0,  // Gray
        Common    = 1,  // White
        Uncommon  = 2,  // Green
        Rare      = 3,  // Blue
        Epic      = 4,  // Purple
        Legendary = 5,  // Orange
    };

    // Equipment types (matches equip_type DB column)
    enum class EquipType : int
    {
        None         = 0,
        Head         = 1,
        Chest        = 2,
        Legs         = 3,
        Feet         = 4,
        Hands        = 5,
        Weapon       = 6,
        Offhand      = 7,
        Ranged       = 8,
        Ring         = 9,
        Amulet       = 10,
        Consumable   = 11,
        QuestItem    = 12,
        Gem          = 13,
        Misc         = 14,
    };

    // Weapon types (matches weapon_type DB column)
    enum class WeaponType : int
    {
        None    = 0,
        Sword   = 1,
        Axe     = 2,
        Mace    = 3,
        Dagger  = 4,
        Staff   = 5,
        Bow     = 6,
        Wand    = 7,
        Shield  = 8,
    };

    // Weapon material (affects sound effects)
    enum class WeaponMaterial : int
    {
        None    = 0,
        Metal   = 1,
        Wood    = 2,
        Bone    = 3,
    };

    // Armor types
    enum class ArmorType : int
    {
        NullArmor     = 0,
        Cloth         = 1,
        Leather       = 2,
        Mail          = 3,
        Plate         = 4,
    };

    // Item flags
    enum ItemFlags : uint32_t
    {
        Flag_None         = 0x00,
        Flag_Soulbound    = 0x01,
        Flag_Unique       = 0x02,
        Flag_Stackable    = 0x04,
        Flag_Generated    = 0x08,
        Flag_QuestItem    = 0x10,
    };

    // Item definition identifier (database entry ID)
    using ItemDefinition = int;

    // Magic school types (for damage/resistance)
    enum class MagicSchool : int
    {
        NullMagicSchool = 0,
        Physical        = 1,
        Fire            = 2,
        Frost           = 3,
        Shadow          = 4,
        Holy            = 5,
        Magic           = 6,
    };
}
