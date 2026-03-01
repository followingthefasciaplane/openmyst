#pragma once

#include <cstdint>

namespace Conditions
{
    // Condition types for spell/quest/script evaluation
    enum class Type : int
    {
        None               = 0,
        HasItem            = 1,
        HasAura            = 2,
        IsAlive            = 3,
        IsDead             = 4,
        HealthPct          = 5,
        ManaPct            = 6,
        InCombat           = 7,
        HasQuest           = 8,
        QuestComplete      = 9,
        ClassEquals        = 10,
        LevelMin           = 11,
        LevelMax           = 12,
        HasGuild           = 13,
        InMap              = 14,
    };

    // Comparison operators for condition evaluation
    enum class Compare : int
    {
        Equal              = 0,
        NotEqual           = 1,
        GreaterThan        = 2,
        LessThan           = 3,
        GreaterOrEqual     = 4,
        LessOrEqual        = 5,
    };
}
