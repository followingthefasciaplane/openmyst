#pragma once
#include "core/Types.h"

class ServerUnit;

namespace Combat {
    // Damage calculation
    int32 calculatePhysicalDamage(ServerUnit* attacker, ServerUnit* victim, int32 baseDamage);
    int32 calculateSpellDamage(ServerUnit* attacker, ServerUnit* victim, int32 baseDamage, uint32 school);
    int32 calculateHeal(ServerUnit* caster, ServerUnit* target, int32 baseHeal);

    // Armor mitigation
    float getArmorReduction(int32 armor, uint16 attackerLevel);

    // Scaling formulas from binary
    float getSpellScaling(ServerUnit* caster, uint32 scaleFormula, uint32 statScale1, uint32 statScale2);

    // Level difference modifier
    float getLevelDiffModifier(uint16 attackerLevel, uint16 defenderLevel);

    // Combat checks
    bool isHostile(ServerUnit* a, ServerUnit* b);
    bool canAttack(ServerUnit* attacker, ServerUnit* victim);
    float getDistance(ServerUnit* a, ServerUnit* b);

    // XP from kill
    uint32 calculateKillXp(uint16 killerLevel, uint16 victimLevel, bool isElite, bool isBoss);
    uint32 getMoneyDrop(uint16 npcLevel, float customGoldRatio);
}
