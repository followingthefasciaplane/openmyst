#pragma once

#include "ItemDefines.h"
#include "ItemAffix.h"
#include <string>

// Defines a specific item instance (base template + affixes + sockets)
struct ItemDefiner
{
    int                       entry       = 0;
    int                       sortEntry   = 0;
    int                       itemLevel   = 0;
    int                       requiredLevel = 0;
    ItemDefines::Quality      quality     = ItemDefines::Quality::Common;
    ItemDefines::EquipType    equipType   = ItemDefines::EquipType::None;
    ItemDefines::WeaponType   weaponType  = ItemDefines::WeaponType::None;
    ItemDefines::ArmorType    armorType   = ItemDefines::ArmorType::NullArmor;
    ItemDefines::WeaponMaterial weaponMaterial = ItemDefines::WeaponMaterial::None;
    int                       durability  = 0;
    int                       sellPrice   = 0;
    int                       stackCount  = 1;
    int                       numSockets  = 0;
    int                       requiredClass = 0;
    int                       questOffer  = 0;
    uint32_t                  flags       = 0;
    bool                      generated   = false;
    int                       buyPrice    = 0;

    std::string               name;
    std::string               description;
    std::string               iconName;

    // Affixes (for generated items)
    ItemAffix                 prefix;
    ItemAffix                 suffix;

    // Weapon/armor stats
    int                       weaponValue      = 0;
    int                       rangedWeaponValue = 0;
    int                       armorValue       = 0;

    bool isValid() const { return entry > 0; }
};
