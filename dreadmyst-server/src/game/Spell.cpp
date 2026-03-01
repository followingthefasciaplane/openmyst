#include "game/Spell.h"
#include "game/ServerUnit.h"
#include "game/ServerPlayer.h"
#include "game/ServerNpc.h"
#include "game/ServerMap.h"
#include "game/GameCache.h"
#include "game/Combat.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"
#include "core/Log.h"
#include "core/Server.h"

#include <cmath>
#include <algorithm>
#include <ctime>

// ---------------------------------------------------------------------------
//  Construction
// ---------------------------------------------------------------------------

Spell::Spell(ServerUnit* caster, const SpellTemplate* tmpl)
    : m_caster(caster)
    , m_template(tmpl)
{
}

// ---------------------------------------------------------------------------
//  Validation
// ---------------------------------------------------------------------------

SpellCastResult Spell::validate()
{
    if (!m_caster || !m_template)
        return SpellCastResult::FailedRequirement;

    // Caster must be alive
    if (!m_caster->isAlive())
        return SpellCastResult::CasterDead;

    // Validate target for single-target spells
    for (int i = 0; i < MAX_SPELL_EFFECTS; ++i) {
        const SpellEffect& eff = m_template->effects[i];
        if (eff.type == 0)
            continue;

        // targetType 1 = SingleTarget -- needs a valid, alive target
        if (eff.targetType == 1) {
            if (m_targetGuid == 0)
                return SpellCastResult::InvalidTarget;

            ServerMap* map = m_caster->getMap();
            if (!map)
                return SpellCastResult::InvalidTarget;

            // Look for target among players and units on the map
            ServerUnit* target = map->findPlayer(m_targetGuid);
            if (!target) {
                // Try NPC lookup -- findNpc returns ServerNpc* which is a ServerUnit*
                auto npcs = map->getNpcs();
                for (auto& npc : npcs) {
                    if (npc && npc->getGuid() == m_targetGuid) {
                        target = npc.get();
                        break;
                    }
                }
            }
            if (!target)
                return SpellCastResult::InvalidTarget;

            // For damage/hostile effects the target must be alive
            if (!eff.positive && !target->isAlive())
                return SpellCastResult::TargetDead;
            break; // Only need to validate once for single-target
        }
    }

    SpellCastResult res;

    res = checkMana();
    if (res != SpellCastResult::Success)
        return res;

    res = checkCooldown();
    if (res != SpellCastResult::Success)
        return res;

    res = checkRange();
    if (res != SpellCastResult::Success)
        return res;

    return SpellCastResult::Success;
}

SpellCastResult Spell::checkMana()
{
    if (!m_caster || !m_template)
        return SpellCastResult::FailedRequirement;

    int32 manaCost = 0;

    // Flat mana cost from formula field
    manaCost += static_cast<int32>(m_template->manaFormula);

    // Percentage of max mana
    if (m_template->manaPct > 0.0f) {
        manaCost += static_cast<int32>(m_caster->getMaxMana() * m_template->manaPct / 100.0f);
    }

    if (manaCost > 0 && m_caster->getMana() < manaCost)
        return SpellCastResult::NotEnoughMana;

    return SpellCastResult::Success;
}

SpellCastResult Spell::checkCooldown()
{
    if (!m_caster || !m_template)
        return SpellCastResult::FailedRequirement;

    // Only players track spell cooldowns
    ServerPlayer* player = dynamic_cast<ServerPlayer*>(m_caster);
    if (player) {
        if (player->isSpellOnCooldown(m_template->entry))
            return SpellCastResult::OnCooldown;
    }

    return SpellCastResult::Success;
}

SpellCastResult Spell::checkRange()
{
    if (!m_caster || !m_template)
        return SpellCastResult::FailedRequirement;

    // Range of 0 means unlimited / self-cast
    if (m_template->range <= 0.0f)
        return SpellCastResult::Success;

    // Need a target to check range against
    if (m_targetGuid == 0)
        return SpellCastResult::Success; // AoE at position or self-cast

    ServerMap* map = m_caster->getMap();
    if (!map)
        return SpellCastResult::InvalidTarget;

    // Find target unit
    ServerUnit* target = nullptr;
    target = map->findPlayer(m_targetGuid);
    if (!target) {
        auto npcs = map->getNpcs();
        for (auto& npc : npcs) {
            if (npc && npc->getGuid() == m_targetGuid) {
                target = npc.get();
                break;
            }
        }
    }
    if (!target)
        return SpellCastResult::InvalidTarget;

    float dist = Combat::getDistance(m_caster, target);

    if (dist > m_template->range)
        return SpellCastResult::OutOfRange;

    if (m_template->rangeMin > 0.0f && dist < m_template->rangeMin)
        return SpellCastResult::OutOfRange;

    return SpellCastResult::Success;
}

// ---------------------------------------------------------------------------
//  Execution flow
// ---------------------------------------------------------------------------

void Spell::prepare()
{
    if (!m_caster || !m_template) {
        m_finished = true;
        return;
    }

    if (m_template->castTime > 0) {
        m_castTimer = static_cast<int32>(m_template->castTime);
        m_isCasting = true;
        LOG_DEBUG("Spell::prepare - spell %u cast time %u ms", m_template->entry, m_template->castTime);
    } else {
        // Instant cast -- execute immediately
        cast();
    }
}

void Spell::cast()
{
    if (!m_caster || !m_template) {
        m_finished = true;
        return;
    }

    m_isCasting = false;

    LOG_DEBUG("Spell::cast - caster %u casting spell %u '%s'",
              m_caster->getGuid(), m_template->entry, m_template->name.c_str());

    // Process each effect slot
    for (int i = 0; i < MAX_SPELL_EFFECTS; ++i) {
        const SpellEffect& eff = m_template->effects[i];
        if (eff.type == 0)
            continue;

        std::vector<ServerUnit*> targets = resolveTargets(i);
        for (ServerUnit* tgt : targets) {
            if (!tgt)
                continue;
            executeEffect(i, tgt);
        }
    }

    // Post-cast housekeeping
    consumeMana();
    triggerCooldown();
    sendSpellGo();
    finish();
}

void Spell::finish()
{
    m_finished  = true;
    m_isCasting = false;
}

void Spell::cancel()
{
    LOG_DEBUG("Spell::cancel - spell %u cancelled", m_template ? m_template->entry : 0);
    m_finished  = true;
    m_isCasting = false;
}

void Spell::update(int32 deltaMs)
{
    if (m_finished)
        return;

    if (m_isCasting) {
        m_castTimer -= deltaMs;
        if (m_castTimer <= 0) {
            m_castTimer = 0;
            cast();
        }
    }
}

// ---------------------------------------------------------------------------
//  Effect execution (main switch)
// ---------------------------------------------------------------------------

void Spell::executeEffect(int effectIndex, ServerUnit* target)
{
    if (!m_caster || !m_template || !target)
        return;

    const SpellEffect& eff = m_template->effects[effectIndex];
    SpellEffectType type = static_cast<SpellEffectType>(eff.type);

    switch (type) {

    // ---- SchoolDamage (1) ----
    case SpellEffectType::SchoolDamage: {
        float scaling = Combat::getSpellScaling(m_caster, eff.scaleFormula,
                                                 m_template->statScale1,
                                                 m_template->statScale2);
        int32 baseDmg = eff.data1 + static_cast<int32>(scaling);
        int32 damage = Combat::calculateSpellDamage(m_caster, target, baseDmg,
                                                     m_template->castSchool);
        if (damage > 0) {
            target->modifyHealth(-damage);
            target->onDamageTaken(m_caster, damage);
        }
        LOG_DEBUG("Spell effect SchoolDamage: %d damage to unit %u", damage, target->getGuid());
        break;
    }

    // ---- WeaponDamage (2) ----
    case SpellEffectType::WeaponDamage: {
        // Base weapon damage + bonus from effect data
        int32 baseDmg = eff.data1;
        int32 damage = Combat::calculatePhysicalDamage(m_caster, target, baseDmg);
        if (damage > 0) {
            target->modifyHealth(-damage);
            target->onDamageTaken(m_caster, damage);
        }
        LOG_DEBUG("Spell effect WeaponDamage: %d damage to unit %u", damage, target->getGuid());
        break;
    }

    // ---- MeleeAtk (3) ----
    case SpellEffectType::MeleeAtk: {
        int32 baseDmg = eff.data1;
        int32 damage = Combat::calculatePhysicalDamage(m_caster, target, baseDmg);
        if (damage > 0) {
            target->modifyHealth(-damage);
            target->onDamageTaken(m_caster, damage);
        }
        LOG_DEBUG("Spell effect MeleeAtk: %d damage to unit %u", damage, target->getGuid());
        break;
    }

    // ---- RangedAtk (4) ----
    case SpellEffectType::RangedAtk: {
        int32 baseDmg = eff.data1;
        int32 damage = Combat::calculatePhysicalDamage(m_caster, target, baseDmg);
        if (damage > 0) {
            target->modifyHealth(-damage);
            target->onDamageTaken(m_caster, damage);
        }
        LOG_DEBUG("Spell effect RangedAtk: %d damage to unit %u", damage, target->getGuid());
        break;
    }

    // ---- Heal (5) ----
    case SpellEffectType::Heal: {
        float scaling = Combat::getSpellScaling(m_caster, eff.scaleFormula,
                                                 m_template->statScale1,
                                                 m_template->statScale2);
        int32 baseHeal = eff.data1 + static_cast<int32>(scaling);
        int32 heal = Combat::calculateHeal(m_caster, target, baseHeal);
        if (heal > 0) {
            target->modifyHealth(heal);
            target->onHealReceived(m_caster, heal);
        }
        LOG_DEBUG("Spell effect Heal: %d healing to unit %u", heal, target->getGuid());
        break;
    }

    // ---- HealPct (6) ----
    case SpellEffectType::HealPct: {
        int32 heal = target->getMaxHealth() * eff.data1 / 100;
        if (heal > 0) {
            target->modifyHealth(heal);
            target->onHealReceived(m_caster, heal);
        }
        LOG_DEBUG("Spell effect HealPct: %d%% = %d healing to unit %u", eff.data1, heal, target->getGuid());
        break;
    }

    // ---- HealthDrain (7) ----
    case SpellEffectType::HealthDrain: {
        float scaling = Combat::getSpellScaling(m_caster, eff.scaleFormula,
                                                 m_template->statScale1,
                                                 m_template->statScale2);
        int32 drain = eff.data1 + static_cast<int32>(scaling);
        drain = Combat::calculateSpellDamage(m_caster, target, drain, m_template->castSchool);
        if (drain > 0) {
            // Clamp drain to target's current health
            drain = std::min(drain, target->getHealth());
            target->modifyHealth(-drain);
            target->onDamageTaken(m_caster, drain);
            m_caster->modifyHealth(drain);
        }
        LOG_DEBUG("Spell effect HealthDrain: %d from unit %u to caster %u",
                  drain, target->getGuid(), m_caster->getGuid());
        break;
    }

    // ---- ManaDrain (8) ----
    case SpellEffectType::ManaDrain: {
        int32 drain = eff.data1;
        drain = std::min(drain, target->getMana());
        if (drain > 0) {
            target->modifyMana(-drain);
            m_caster->modifyMana(drain);
        }
        LOG_DEBUG("Spell effect ManaDrain: %d mana from unit %u", drain, target->getGuid());
        break;
    }

    // ---- ManaBurn (9) ----
    case SpellEffectType::ManaBurn: {
        int32 burn = eff.data1;
        burn = std::min(burn, target->getMana());
        if (burn > 0) {
            target->modifyMana(-burn);
            // Deal damage equal to mana burned
            target->modifyHealth(-burn);
            target->onDamageTaken(m_caster, burn);
        }
        LOG_DEBUG("Spell effect ManaBurn: %d mana burned + %d damage to unit %u",
                  burn, burn, target->getGuid());
        break;
    }

    // ---- RestoreMana (10) ----
    case SpellEffectType::RestoreMana: {
        int32 restore = eff.data1;
        if (restore > 0) {
            target->modifyMana(restore);
        }
        LOG_DEBUG("Spell effect RestoreMana: %d mana to unit %u", restore, target->getGuid());
        break;
    }

    // ---- RestoreManaPct (11) ----
    case SpellEffectType::RestoreManaPct: {
        int32 restore = target->getMaxMana() * eff.data1 / 100;
        if (restore > 0) {
            target->modifyMana(restore);
        }
        LOG_DEBUG("Spell effect RestoreManaPct: %d%% = %d mana to unit %u",
                  eff.data1, restore, target->getGuid());
        break;
    }

    // ---- ApplyAura (12) ----
    case SpellEffectType::ApplyAura: {
        applyAura(effectIndex, target);
        break;
    }

    // ---- ApplyAreaAura (13) ----
    case SpellEffectType::ApplyAreaAura: {
        applyAura(effectIndex, target);
        break;
    }

    // ---- Charge (14) ----
    case SpellEffectType::Charge: {
        // Move caster to target position
        m_caster->setPosition(target->getPositionX(), target->getPositionY());
        LOG_DEBUG("Spell effect Charge: caster %u charged to unit %u",
                  m_caster->getGuid(), target->getGuid());
        break;
    }

    // ---- KnockBack (15) ----
    case SpellEffectType::KnockBack: {
        LOG_DEBUG("Spell effect KnockBack: on unit %u (data1=%d)", target->getGuid(), eff.data1);
        break;
    }

    // ---- PullTo (16) ----
    case SpellEffectType::PullTo: {
        target->setPosition(m_caster->getPositionX(), m_caster->getPositionY());
        LOG_DEBUG("Spell effect PullTo: unit %u pulled to caster %u",
                  target->getGuid(), m_caster->getGuid());
        break;
    }

    // ---- SlideFrom (17) ----
    case SpellEffectType::SlideFrom: {
        LOG_DEBUG("Spell effect SlideFrom: on unit %u (data1=%d)", target->getGuid(), eff.data1);
        break;
    }

    // ---- Teleport (18) ----
    case SpellEffectType::Teleport: {
        // Teleport to the specified target position
        target->setPosition(m_targetX, m_targetY);
        LOG_DEBUG("Spell effect Teleport: unit %u to (%.1f, %.1f)",
                  target->getGuid(), m_targetX, m_targetY);
        break;
    }

    // ---- TeleportForward (19) ----
    case SpellEffectType::TeleportForward: {
        LOG_DEBUG("Spell effect TeleportForward: on unit %u (distance=%d)",
                  target->getGuid(), eff.data1);
        break;
    }

    // ---- LearnSpell (20) ----
    case SpellEffectType::LearnSpell: {
        ServerPlayer* player = dynamic_cast<ServerPlayer*>(target);
        if (player && eff.data1 > 0) {
            player->learnSpell(static_cast<uint32>(eff.data1));
            LOG_DEBUG("Spell effect LearnSpell: player %u learned spell %d",
                      player->getGuid(), eff.data1);
        }
        break;
    }

    // ---- TriggerSpell (21) ----
    case SpellEffectType::TriggerSpell: {
        if (eff.data1 > 0) {
            const GameCache* cache = Server::instance().getGameCache();
            if (cache) {
                const SpellTemplate* triggered = cache->getSpellTemplate(static_cast<uint32>(eff.data1));
                if (triggered) {
                    Spell triggerSpell(m_caster, triggered);
                    triggerSpell.setTarget(target->getGuid());
                    triggerSpell.setTargetPos(m_targetX, m_targetY);
                    triggerSpell.setSpellLevel(m_spellLevel);

                    SpellCastResult res = triggerSpell.validate();
                    if (res == SpellCastResult::Success) {
                        triggerSpell.prepare();
                    }
                    LOG_DEBUG("Spell effect TriggerSpell: triggered spell %d (result=%u)",
                              eff.data1, static_cast<uint32>(res));
                }
            }
        }
        break;
    }

    // ---- CreateItem (22) ----
    case SpellEffectType::CreateItem: {
        ServerPlayer* player = dynamic_cast<ServerPlayer*>(target);
        if (player && eff.data1 > 0) {
            ItemInstance item;
            item.entry = static_cast<uint32>(eff.data1);
            item.count = (eff.data2 > 0) ? static_cast<uint32>(eff.data2) : 1;
            if (player->addItemToInventory(item)) {
                LOG_DEBUG("Spell effect CreateItem: gave item %d x%u to player %u",
                          eff.data1, item.count, player->getGuid());
            } else {
                LOG_WARN("Spell effect CreateItem: player %u inventory full for item %d",
                         player->getGuid(), eff.data1);
            }
        }
        break;
    }

    // ---- CombineItem (23) ----
    case SpellEffectType::CombineItem: {
        LOG_DEBUG("Spell effect CombineItem: on unit %u (data1=%d, data2=%d)",
                  target->getGuid(), eff.data1, eff.data2);
        break;
    }

    // ---- DestroyGems (24) ----
    case SpellEffectType::DestroyGems: {
        LOG_DEBUG("Spell effect DestroyGems: on unit %u (data1=%d)",
                  target->getGuid(), eff.data1);
        break;
    }

    // ---- ExtractOrb (25) ----
    case SpellEffectType::ExtractOrb: {
        LOG_DEBUG("Spell effect ExtractOrb: on unit %u (data1=%d)",
                  target->getGuid(), eff.data1);
        break;
    }

    // ---- LootEffect (26) ----
    case SpellEffectType::LootEffect: {
        LOG_DEBUG("Spell effect LootEffect: on unit %u (data1=%d)",
                  target->getGuid(), eff.data1);
        break;
    }

    // ---- Dispel (27) ----
    case SpellEffectType::Dispel: {
        // Remove auras matching the dispel school (data1)
        uint32 dispelSchool = static_cast<uint32>(eff.data1);
        const auto& auras = target->getAuras();
        const GameCache* cache = Server::instance().getGameCache();
        if (cache) {
            // Iterate in reverse to safely remove
            for (int a = static_cast<int>(auras.size()) - 1; a >= 0; --a) {
                const SpellTemplate* auraTmpl = cache->getSpellTemplate(auras[a].spellId);
                if (auraTmpl && auraTmpl->castSchool == dispelSchool) {
                    target->removeAura(auras[a].spellId);
                    LOG_DEBUG("Spell effect Dispel: removed aura %u (school %u) from unit %u",
                              auras[a].spellId, dispelSchool, target->getGuid());
                    break; // Dispel one aura per effect
                }
            }
        }
        break;
    }

    // ---- Duel (28) ----
    case SpellEffectType::Duel: {
        LOG_DEBUG("Spell effect Duel: caster %u challenges unit %u",
                  m_caster->getGuid(), target->getGuid());
        break;
    }

    // ---- Gossip (29) ----
    case SpellEffectType::Gossip: {
        LOG_DEBUG("Spell effect Gossip: on unit %u (data1=%d)",
                  target->getGuid(), eff.data1);
        break;
    }

    // ---- Inspect (30) ----
    case SpellEffectType::Inspect: {
        LOG_DEBUG("Spell effect Inspect: caster %u inspects unit %u",
                  m_caster->getGuid(), target->getGuid());
        break;
    }

    // ---- InterruptCast (31) ----
    case SpellEffectType::InterruptCast: {
        LOG_DEBUG("Spell effect InterruptCast: on unit %u", target->getGuid());
        break;
    }

    // ---- Kill (32) ----
    case SpellEffectType::Kill: {
        target->setHealth(0);
        target->onDeath(m_caster);
        LOG_DEBUG("Spell effect Kill: unit %u killed by caster %u",
                  target->getGuid(), m_caster->getGuid());
        break;
    }

    // ---- SummonNpc (33) ----
    case SpellEffectType::SummonNpc: {
        LOG_DEBUG("Spell effect SummonNpc: entry=%d at caster %u position",
                  eff.data1, m_caster->getGuid());
        break;
    }

    // ---- SummonObject (34) ----
    case SpellEffectType::SummonObject: {
        LOG_DEBUG("Spell effect SummonObject: entry=%d at caster %u position",
                  eff.data1, m_caster->getGuid());
        break;
    }

    // ---- Resurrect (35) ----
    case SpellEffectType::Resurrect: {
        if (!target->isAlive()) {
            int32 restoreHp = eff.data1;
            if (restoreHp <= 0)
                restoreHp = target->getMaxHealth() / 2; // Default to 50% HP
            target->setHealth(std::min(restoreHp, target->getMaxHealth()));
            LOG_DEBUG("Spell effect Resurrect: unit %u restored with %d HP",
                      target->getGuid(), restoreHp);
        }
        break;
    }

    // ---- Threat (36) ----
    case SpellEffectType::Threat: {
        LOG_DEBUG("Spell effect Threat: %d threat on unit %u from caster %u",
                  eff.data1, target->getGuid(), m_caster->getGuid());
        break;
    }

    // ---- NearestWayp (37) ----
    case SpellEffectType::NearestWayp: {
        LOG_DEBUG("Spell effect NearestWayp: on unit %u", target->getGuid());
        break;
    }

    // ---- ScriptEffect (38) ----
    case SpellEffectType::ScriptEffect: {
        LOG_DEBUG("Spell effect ScriptEffect: script=%d on unit %u",
                  eff.data1, target->getGuid());
        break;
    }

    // ---- ApplyGemSocket (39) ----
    case SpellEffectType::ApplyGemSocket: {
        LOG_DEBUG("Spell effect ApplyGemSocket: on unit %u (data1=%d)",
                  target->getGuid(), eff.data1);
        break;
    }

    // ---- ApplyOrbEnchant (40) ----
    case SpellEffectType::ApplyOrbEnchant: {
        LOG_DEBUG("Spell effect ApplyOrbEnchant: on unit %u (data1=%d)",
                  target->getGuid(), eff.data1);
        break;
    }

    // ---- NullEffect / Unknown ----
    case SpellEffectType::NullEffect:
    default:
        LOG_DEBUG("Spell::executeEffect - unimplemented effect type %u for spell %u",
                  eff.type, m_template->entry);
        break;
    }
}

// ---------------------------------------------------------------------------
//  Aura application
// ---------------------------------------------------------------------------

void Spell::applyAura(int effectIndex, ServerUnit* target)
{
    if (!m_caster || !m_template || !target)
        return;

    const SpellEffect& eff = m_template->effects[effectIndex];

    Aura aura;
    aura.spellId    = m_template->entry;
    aura.casterGuid = m_caster->getGuid();
    aura.stackCount = 1;
    aura.spellLevel = m_spellLevel;
    aura.casterLevel = static_cast<int32>(m_caster->getLevel());

    // Duration from spell template
    aura.remainingMs = static_cast<int32>(m_template->duration);

    // Set up tick data for this effect slot
    AuraTickData& tickData = aura.effects[effectIndex];
    tickData.casterGuid    = m_caster->getGuid();
    tickData.tickTotal     = static_cast<float>(eff.data1);
    tickData.tickAmount    = 0.0f;
    tickData.tickAmountTicked = 0.0f;

    // Configure periodic tick interval from the spell template
    if (m_template->interval > 0) {
        tickData.tickIntervalMs = static_cast<int32>(m_template->interval);
        tickData.tickTimer      = tickData.tickIntervalMs;

        // Calculate number of ticks and per-tick amount
        if (aura.remainingMs > 0 && tickData.tickIntervalMs > 0) {
            tickData.numTicks    = aura.remainingMs / tickData.tickIntervalMs;
            tickData.numTicksCounter = 0;
            if (tickData.numTicks > 0) {
                tickData.tickAmount = tickData.tickTotal / static_cast<float>(tickData.numTicks);
            }
        }
    } else {
        // Non-periodic aura -- flat modifier
        tickData.tickAmount     = static_cast<float>(eff.data1);
        tickData.numTicks       = 0;
        tickData.tickIntervalMs = 0;
        tickData.tickTimer      = 0;
    }

    target->addAura(aura);

    LOG_DEBUG("Spell::applyAura - spell %u aura applied to unit %u (duration=%d ms, interval=%u ms)",
              m_template->entry, target->getGuid(), aura.remainingMs, m_template->interval);
}

// ---------------------------------------------------------------------------
//  Target resolution
// ---------------------------------------------------------------------------

std::vector<ServerUnit*> Spell::resolveTargets(int effectIndex)
{
    std::vector<ServerUnit*> targets;

    if (!m_caster || !m_template)
        return targets;

    const SpellEffect& eff = m_template->effects[effectIndex];

    switch (eff.targetType) {

    // Self (0): caster targets itself
    case 0:
        targets.push_back(m_caster);
        break;

    // SingleTarget (1): find unit by guid
    case 1: {
        if (m_targetGuid == 0)
            break;

        ServerMap* map = m_caster->getMap();
        if (!map)
            break;

        // Check players first
        ServerUnit* target = map->findPlayer(m_targetGuid);
        if (!target) {
            // Check NPCs
            auto npcs = map->getNpcs();
            for (auto& npc : npcs) {
                if (npc && npc->getGuid() == m_targetGuid) {
                    target = npc.get();
                    break;
                }
            }
        }
        if (target) {
            targets.push_back(target);
        }
        break;
    }

    // AreaRadius (2): all units in range from target position or caster
    case 2: {
        ServerMap* map = m_caster->getMap();
        if (!map)
            break;

        float cx = (m_targetX != 0.0f || m_targetY != 0.0f) ? m_targetX : m_caster->getPositionX();
        float cy = (m_targetX != 0.0f || m_targetY != 0.0f) ? m_targetY : m_caster->getPositionY();
        float radius = eff.radius;
        if (radius <= 0.0f)
            radius = m_template->range;

        auto units = map->getUnitsInRange(cx, cy, radius);

        uint32 maxTgts = m_template->maxTargets;
        uint32 count = 0;
        for (ServerUnit* u : units) {
            if (!u || !u->isAlive())
                continue;
            targets.push_back(u);
            ++count;
            if (maxTgts > 0 && count >= maxTgts)
                break;
        }
        break;
    }

    default:
        LOG_DEBUG("Spell::resolveTargets - unknown targetType %u for effect %d of spell %u",
                  eff.targetType, effectIndex, m_template->entry);
        break;
    }

    return targets;
}

// ---------------------------------------------------------------------------
//  Packet sending
// ---------------------------------------------------------------------------

void Spell::sendSpellGo()
{
    if (!m_caster || !m_template)
        return;

    ServerMap* map = m_caster->getMap();
    if (!map)
        return;

    GamePacket pkt(SMSG_SPELL_GO);
    pkt.writeUint32(m_caster->getGuid());
    pkt.writeUint32(m_targetGuid);
    pkt.writeUint32(m_template->entry);

    map->broadcastPacket(pkt);
}

void Spell::sendCastResult(SpellCastResult result)
{
    if (!m_caster || !m_template)
        return;

    ServerPlayer* player = dynamic_cast<ServerPlayer*>(m_caster);
    if (!player)
        return;

    GamePacket pkt(SMSG_COMBAT_MSG);
    pkt.writeUint32(m_caster->getGuid());
    pkt.writeUint32(m_template->entry);
    pkt.writeUint32(static_cast<uint32>(result));

    player->sendPacket(pkt);
}

// ---------------------------------------------------------------------------
//  Resource consumption
// ---------------------------------------------------------------------------

void Spell::consumeMana()
{
    if (!m_caster || !m_template)
        return;

    int32 manaCost = 0;

    // Flat mana cost from formula field
    manaCost += static_cast<int32>(m_template->manaFormula);

    // Percentage of max mana
    if (m_template->manaPct > 0.0f) {
        manaCost += static_cast<int32>(m_caster->getMaxMana() * m_template->manaPct / 100.0f);
    }

    if (manaCost > 0) {
        m_caster->modifyMana(-manaCost);
    }

    // Health cost if applicable
    if (m_template->healthCost > 0) {
        m_caster->modifyHealth(-static_cast<int32>(m_template->healthCost));
    }
    if (m_template->healthPctCost > 0.0f) {
        int32 hpCost = static_cast<int32>(m_caster->getMaxHealth() * m_template->healthPctCost / 100.0f);
        m_caster->modifyHealth(-hpCost);
    }
}

void Spell::triggerCooldown()
{
    if (!m_caster || !m_template)
        return;

    if (m_template->cooldown == 0)
        return;

    ServerPlayer* player = dynamic_cast<ServerPlayer*>(m_caster);
    if (player) {
        int64 now = Server::instance().getUnixTime();
        int64 cooldownSecs = static_cast<int64>(m_template->cooldown) / 1000; // Convert ms to seconds
        player->addSpellCooldown(m_template->entry, now, now + cooldownSecs);
    }
}
