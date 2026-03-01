#pragma once
#include "core/Types.h"
#include <string>

// Spell effect data (3 effects per spell)
struct SpellEffect {
    uint32 type           = 0;   // SpellEffectType enum
    int32  data1          = 0;
    int32  data2          = 0;
    int32  data3          = 0;
    uint32 targetType     = 0;
    float  radius         = 0.0f;
    uint8  positive       = 0;
    uint32 scaleFormula   = 0;
};

// Spell template loaded from game.db spell_template table
// Exact columns verified from binary string at 0x0058b1e0
struct SpellTemplate {
    uint32 entry              = 0;
    std::string name;
    uint32 manaFormula        = 0;
    float  manaPct            = 0.0f;
    SpellEffect effects[MAX_SPELL_EFFECTS];
    uint32 maxTargets         = 0;
    uint32 dispel             = 0;
    uint32 attributes         = 0;
    uint32 castTime           = 0;
    uint32 cooldown           = 0;
    uint32 cooldownCategory   = 0;
    uint32 auraInterruptFlags = 0;
    uint32 duration           = 0;
    uint32 durationFormula    = 0;
    float  speed              = 0.0f;
    uint32 castInterruptFlags = 0;
    uint32 preventionType     = 0;
    uint32 castSchool         = 0;
    uint32 magicRollSchool    = 0;
    float  range              = 0.0f;
    float  rangeMin           = 0.0f;
    uint32 stackAmount        = 0;
    uint32 requiredEquipment  = 0;
    uint32 healthCost         = 0;
    float  healthPctCost      = 0.0f;
    uint32 activatedByIn      = 0;
    uint32 activatedByOut     = 0;
    uint32 interval           = 0;
    uint32 reqCasterMechanic  = 0;
    uint32 reqTarMechanic     = 0;
    uint32 reqCasterAura      = 0;
    uint32 reqTarAura         = 0;
    uint32 statScale1         = 0;
    uint32 statScale2         = 0;
    uint32 canLevelUp         = 0;

    // From spell_visual join
    uint32 unitGoAnimation    = 0;
    uint32 unitCastAnimation  = 0;
};

// Spell visual kit (from spell_visual_kit table)
struct SpellVisualKit {
    uint32 entry         = 0;
    uint32 animationId   = 0;
    uint32 projectileId  = 0;
    uint32 impactId      = 0;
};
