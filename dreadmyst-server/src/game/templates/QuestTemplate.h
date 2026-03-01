#pragma once
#include "core/Types.h"

// Quest template loaded from game.db quest_template
// Exact columns verified from binary string at 0x005888b0
struct QuestTemplate {
    uint32 entry            = 0;
    uint32 minLevel         = 0;
    uint32 flags            = 0;
    uint32 prevQuest[MAX_QUEST_PREREQS] = {};

    uint32 providedItem     = 0;

    // Requirements (items, npcs, game objects, spells) with counts
    uint32 reqItem[MAX_QUEST_OBJECTIVES]  = {};
    uint32 reqNpc[MAX_QUEST_OBJECTIVES]   = {};
    uint32 reqGo[MAX_QUEST_OBJECTIVES]    = {};
    uint32 reqSpell[MAX_QUEST_OBJECTIVES] = {};
    uint32 reqCount[MAX_QUEST_OBJECTIVES] = {};

    // Reward choices (player picks one)
    uint32 rewChoiceItem[MAX_QUEST_REWARD_CHOICES]  = {};
    uint32 rewChoiceCount[MAX_QUEST_REWARD_CHOICES] = {};

    // Fixed rewards (always given)
    uint32 rewItem[MAX_QUEST_REWARDS]      = {};
    uint32 rewItemCount[MAX_QUEST_REWARDS] = {};

    uint32 rewMoney         = 0;
    uint32 rewXp            = 0;
    uint32 startScript      = 0;
    uint32 completeScript   = 0;
    uint32 startNpcEntry    = 0;
    uint32 finishNpcEntry   = 0;
};

// Player quest status (runtime)
struct QuestStatus {
    uint32 questEntry       = 0;
    uint8  rewarded         = 0;
    uint32 objectiveCount[MAX_QUEST_OBJECTIVES] = {};

    bool isComplete(const QuestTemplate& tmpl) const {
        for (int i = 0; i < MAX_QUEST_OBJECTIVES; i++) {
            if (tmpl.reqCount[i] > 0 && objectiveCount[i] < tmpl.reqCount[i])
                return false;
        }
        return true;
    }
};
