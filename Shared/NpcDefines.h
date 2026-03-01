#pragma once

#include <cstdint>

namespace NpcDefines
{
    // NPC movement types
    enum class MovementType : int
    {
        Stationary   = 0,
        Wander       = 1,
        Path         = 2,
    };

    // NPC armor preference (affects model rendering)
    enum class ArmorPreference : int
    {
        None              = 0,
        ArmorHeavy        = 1,
        ArmorCasterCloth  = 2,
    };

    // NPC flags
    enum NpcFlags : uint32_t
    {
        Flag_None        = 0x00,
        Flag_Gossip      = 0x01,
        Flag_Vendor       = 0x02,
        Flag_QuestGiver   = 0x04,
        Flag_Repair       = 0x08,
        Flag_Banker       = 0x10,
        Flag_Trainer      = 0x20,
        Flag_Guard        = 0x40,
    };
}
