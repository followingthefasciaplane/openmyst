#pragma once

#include <cstdint>

namespace GoDefines
{
    // Game object types
    enum class Type : int
    {
        None         = 0,
        Chest        = 1,
        Door         = 2,
        Lever        = 3,
        Waypoint     = 4,
        AreaTrigger  = 5,
        Decoration   = 6,
    };

    // Toggle state for interactive game objects (doors, levers, chests)
    enum class ToggleState : int
    {
        Closed = 0,
        Open   = 1,
    };

    // Game object flags
    enum Flags : uint32_t
    {
        Flag_None           = 0x00,
        Flag_Lootable       = 0x01,
        Flag_Interactable   = 0x02,
        Flag_Uninteractable = 0x04,
        Flag_Locked         = 0x08,
    };
}
