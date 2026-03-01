#pragma once

#include <cstdint>
#include <string>

namespace ScriptDefines
{
    // Script trigger types
    enum class TriggerType : int
    {
        None       = 0,
        OnKill     = 1,
        OnDeath    = 2,
        OnSpawn    = 3,
        OnTalk     = 4,
        OnTimer    = 5,
        OnDamage   = 6,
        OnEnter    = 7,
        OnLeave    = 8,
    };

    // Script action types
    enum class ActionType : int
    {
        None          = 0,
        Say           = 1,
        CastSpell     = 2,
        Summon        = 3,
        Despawn       = 4,
        SetFlag       = 5,
        RemoveFlag    = 6,
        Teleport      = 7,
        GiveItem      = 8,
        GiveXp        = 9,
    };
}
