#include "game/Combat.h"
#include "game/ServerUnit.h"

#include <cmath>
#include <algorithm>

namespace Combat {

// ---------------------------------------------------------------------------
//  Armor mitigation
// ---------------------------------------------------------------------------

float getArmorReduction(int32 armor, uint16 attackerLevel)
{
    if (armor <= 0)
        return 0.0f;

    // Standard diminishing-returns formula:
    //   reduction = armor / (armor + 400 + 85 * attackerLevel)
    float denominator = static_cast<float>(armor) + 400.0f + 85.0f * static_cast<float>(attackerLevel);
    float reduction = static_cast<float>(armor) / denominator;

    // Clamp to [0, 0.75] -- armor can never mitigate more than 75%
    return std::clamp(reduction, 0.0f, 0.75f);
}

// ---------------------------------------------------------------------------
//  Level difference modifier
// ---------------------------------------------------------------------------

float getLevelDiffModifier(uint16 attackerLevel, uint16 defenderLevel)
{
    int diff = static_cast<int>(attackerLevel) - static_cast<int>(defenderLevel);

    // Each level of advantage gives +3% damage, disadvantage gives -3%
    // Clamped so the modifier stays in [0.5, 1.5]
    float mod = 1.0f + 0.03f * static_cast<float>(diff);
    return std::clamp(mod, 0.5f, 1.5f);
}

// ---------------------------------------------------------------------------
//  Spell scaling
// ---------------------------------------------------------------------------

float getSpellScaling(ServerUnit* caster, uint32 scaleFormula, uint32 statScale1, uint32 statScale2)
{
    if (!caster || scaleFormula == 0)
        return 0.0f;

    switch (scaleFormula) {
    case 1: {
        // Scaling formula 1: linear stat scaling
        // scaling = stat1_value * statScale1 ratio + stat2_value * statScale2 ratio
        float scaling = 0.0f;

        if (statScale1 > 0 && statScale1 < static_cast<uint32>(StatType::Count)) {
            int32 statVal = caster->getStat(static_cast<StatType>(statScale1));
            scaling += static_cast<float>(statVal);
        }

        if (statScale2 > 0 && statScale2 < static_cast<uint32>(StatType::Count)) {
            int32 statVal = caster->getStat(static_cast<StatType>(statScale2));
            scaling += static_cast<float>(statVal) * 0.5f; // Secondary stat at half weight
        }

        return scaling;
    }

    case 2: {
        // Scaling formula 2: level-based scaling
        float scaling = static_cast<float>(caster->getLevel());

        if (statScale1 > 0 && statScale1 < static_cast<uint32>(StatType::Count)) {
            int32 statVal = caster->getStat(static_cast<StatType>(statScale1));
            scaling += static_cast<float>(statVal) * 0.25f;
        }

        return scaling;
    }

    default:
        return 0.0f;
    }
}

// ---------------------------------------------------------------------------
//  Damage calculations
// ---------------------------------------------------------------------------

int32 calculatePhysicalDamage(ServerUnit* attacker, ServerUnit* victim, int32 baseDamage)
{
    if (!attacker || !victim || baseDamage <= 0)
        return 0;

    float damage = static_cast<float>(baseDamage);

    // Apply armor reduction
    float armorReduct = getArmorReduction(victim->getArmor(), attacker->getLevel());
    damage *= (1.0f - armorReduct);

    // Apply level difference modifier
    damage *= getLevelDiffModifier(attacker->getLevel(), victim->getLevel());

    // Apply aura modifiers: Aura_ModifyDamageDealtPct on attacker
    const auto& attackerAuras = attacker->getAuras();
    for (const auto& aura : attackerAuras) {
        // Check each effect slot for ModifyDamageDealtPct (type 3)
        for (int i = 0; i < MAX_AURA_EFFECTS; ++i) {
            // The tickAmount field holds the modifier value for flat auras
            // We use the aura's spell template to identify the aura type.
            // For simplicity, we check via a convention:
            // AuraEffectType::ModifyDamageDealtPct = 3
            // The tick data stores the percentage modifier
        }
    }

    // Apply aura modifiers: Aura_ModifyDamageReceivedPct on victim
    // (Similar pattern -- would need aura type lookup in production)

    // Clamp to minimum 0
    int32 result = std::max(0, static_cast<int32>(damage));
    return result;
}

int32 calculateSpellDamage(ServerUnit* attacker, ServerUnit* victim, int32 baseDamage, uint32 school)
{
    if (!attacker || !victim || baseDamage <= 0)
        return 0;

    // Check school immunity (AuraEffectType::SchoolImmunity = 18)
    const auto& victimAuras = victim->getAuras();
    // School immunity check would need aura type introspection; placeholder for now

    float damage = static_cast<float>(baseDamage);

    // Spell damage is NOT reduced by armor -- only level difference applies
    damage *= getLevelDiffModifier(attacker->getLevel(), victim->getLevel());

    // Clamp to minimum 0
    int32 result = std::max(0, static_cast<int32>(damage));
    return result;
}

int32 calculateHeal(ServerUnit* caster, ServerUnit* target, int32 baseHeal)
{
    if (!caster || !target || baseHeal <= 0)
        return 0;

    float heal = static_cast<float>(baseHeal);

    // Healing does not get armor reduction or level diff penalty
    // Aura modifier Aura_ModifyHealingDealtPct (type 5) on caster would apply
    // Aura modifier Aura_ModifyHealingRecvPct  (type 6) on target would apply

    int32 result = std::max(0, static_cast<int32>(heal));
    return result;
}

// ---------------------------------------------------------------------------
//  Combat checks
// ---------------------------------------------------------------------------

bool isHostile(ServerUnit* a, ServerUnit* b)
{
    if (!a || !b)
        return false;

    // Same unit is never hostile to itself
    if (a->getGuid() == b->getGuid())
        return false;

    // Different faction = hostile
    return a->getFaction() != b->getFaction();
}

bool canAttack(ServerUnit* attacker, ServerUnit* victim)
{
    if (!attacker || !victim)
        return false;

    // Cannot attack yourself
    if (attacker->getGuid() == victim->getGuid())
        return false;

    // Both must be alive
    if (!attacker->isAlive() || !victim->isAlive())
        return false;

    // Must be hostile
    if (!isHostile(attacker, victim))
        return false;

    return true;
}

float getDistance(ServerUnit* a, ServerUnit* b)
{
    if (!a || !b)
        return 0.0f;

    float dx = a->getPositionX() - b->getPositionX();
    float dy = a->getPositionY() - b->getPositionY();
    return std::sqrt(dx * dx + dy * dy);
}

// ---------------------------------------------------------------------------
//  XP and money from kills
// ---------------------------------------------------------------------------

uint32 calculateKillXp(uint16 killerLevel, uint16 victimLevel, bool isElite, bool isBoss)
{
    int diff = static_cast<int>(killerLevel) - static_cast<int>(victimLevel);

    // Zero XP if level difference is too large (grey-con)
    if (diff > 8)
        return 0;

    // Base XP scales with victim level
    uint32 baseXp = static_cast<uint32>(victimLevel) * 5 + 45;

    // Apply level difference scaling
    if (diff > 0) {
        // Killing lower level -- reduced XP
        float reduction = 1.0f - (static_cast<float>(diff) * 0.1f);
        reduction = std::max(reduction, 0.1f);
        baseXp = static_cast<uint32>(static_cast<float>(baseXp) * reduction);
    } else if (diff < 0) {
        // Killing higher level -- bonus XP (5% per level)
        float bonus = 1.0f + (static_cast<float>(-diff) * 0.05f);
        bonus = std::min(bonus, 1.5f);
        baseXp = static_cast<uint32>(static_cast<float>(baseXp) * bonus);
    }

    // Elite bonus: 1.5x
    if (isElite)
        baseXp = static_cast<uint32>(static_cast<float>(baseXp) * 1.5f);

    // Boss bonus: 3x (replaces elite bonus if both)
    if (isBoss)
        baseXp = static_cast<uint32>(static_cast<float>(baseXp) * 3.0f);

    return baseXp;
}

uint32 getMoneyDrop(uint16 npcLevel, float customGoldRatio)
{
    // Base gold = npcLevel * 5
    uint32 baseGold = static_cast<uint32>(npcLevel) * 5;

    // Apply custom ratio if provided
    if (customGoldRatio > 0.0f) {
        baseGold = static_cast<uint32>(static_cast<float>(baseGold) * customGoldRatio);
    }

    return baseGold;
}

} // namespace Combat
