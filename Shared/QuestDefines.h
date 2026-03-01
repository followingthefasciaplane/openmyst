#pragma once

#include <cstdint>
#include <string>

namespace QuestDefines
{
    // Quest objective types
    enum class ObjectiveType : int
    {
        None         = 0,
        KillCreature = 1,
        CollectItem  = 2,
        TalkToNpc    = 3,
        ExploreArea  = 4,
        UseGameObj   = 5,
    };

    // Quest tally types (from Server_QuestTally handler at 0x00460760)
    // switch on tallyType byte: 0=NpcKill, 1=ItemCollect, 2=Explore, 3=Interact
    enum TallyType : uint8_t
    {
        TallyNpc     = 0,   // FUN_004e8e80 - kill count
        TallyItem    = 1,   // FUN_004e8d20 - item pickup
        TallyExplore = 2,   // FUN_004e9110 - area exploration
        TallyInteract = 3,  // FUN_004e8fe0 - object interaction
    };

    // Quest status
    enum class Status : int
    {
        NotAvailable   = 0,
        Available      = 1,
        InProgress     = 2,
        Complete       = 3,
        Rewarded       = 4,
        Failed         = 5,
    };

    // Quest flags
    enum QuestFlags : uint32_t
    {
        Flag_None       = 0x00,
        Flag_Repeatable = 0x01,
        Flag_WorldQuest = 0x02,
        Flag_Daily      = 0x04,
    };

    struct Objective
    {
        ObjectiveType type    = ObjectiveType::None;
        int           target  = 0;
        int           count   = 0;
        int           current = 0;
        std::string   description;
    };
}
