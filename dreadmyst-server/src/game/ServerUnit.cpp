#include "game/ServerUnit.h"
#include "game/Spell.h"
#include "core/Log.h"
#include <algorithm>

ServerUnit::~ServerUnit() = default;

void ServerUnit::setHealth(int32 hp) {
    m_health = std::clamp(hp, 0, m_maxHealth);
}

void ServerUnit::setMana(int32 mp) {
    m_mana = std::clamp(mp, 0, m_maxMana);
}

void ServerUnit::modifyHealth(int32 amount) {
    setHealth(m_health + amount);
}

void ServerUnit::modifyMana(int32 amount) {
    setMana(m_mana + amount);
}

int32 ServerUnit::getStat(StatType stat) const {
    int idx = static_cast<int>(stat);
    if (idx < 0 || idx >= static_cast<int>(StatType::Count)) return 0;
    return m_stats[idx];
}

void ServerUnit::setStat(StatType stat, int32 value) {
    int idx = static_cast<int>(stat);
    if (idx < 0 || idx >= static_cast<int>(StatType::Count)) return;
    m_stats[idx] = value;
}

bool ServerUnit::addAura(const Aura& aura) {
    // Check if we already have this aura (stack or refresh)
    for (auto& existing : m_auras) {
        if (existing.spellId == aura.spellId) {
            // Refresh duration and increase stack count
            existing.remainingMs = aura.remainingMs;
            existing.stackCount = std::min(existing.stackCount + 1, (int32)99);
            return true;
        }
    }
    m_auras.push_back(aura);
    return true;
}

void ServerUnit::removeAura(uint32 spellId) {
    m_auras.erase(
        std::remove_if(m_auras.begin(), m_auras.end(),
            [spellId](const Aura& a) { return a.spellId == spellId; }),
        m_auras.end());
}

void ServerUnit::removeAllAuras() {
    m_auras.clear();
}

bool ServerUnit::hasAura(uint32 spellId) const {
    for (const auto& aura : m_auras) {
        if (aura.spellId == spellId) return true;
    }
    return false;
}

void ServerUnit::tickAuras(int32 deltaMs) {
    for (auto it = m_auras.begin(); it != m_auras.end(); ) {
        it->remainingMs -= deltaMs;

        // Tick each effect
        for (int i = 0; i < MAX_AURA_EFFECTS; i++) {
            auto& tick = it->effects[i];
            if (tick.tickIntervalMs <= 0) continue;

            tick.tickTimer += deltaMs;
            while (tick.tickTimer >= tick.tickIntervalMs) {
                tick.tickTimer -= tick.tickIntervalMs;
                tick.numTicksCounter++;
                tick.tickAmountTicked += tick.tickAmount;
                // Apply periodic effect (damage/heal/mana etc)
                // This is handled by the spell system
            }
        }

        if (it->remainingMs <= 0) {
            it = m_auras.erase(it);
        } else {
            ++it;
        }
    }
}

void ServerUnit::update(int32 deltaMs) {
    tickAuras(deltaMs);
}

void ServerUnit::onDeath(ServerUnit* killer) {
    m_health = 0;
    m_inCombat = false;
    removeAllAuras();
}

void ServerUnit::onDamageTaken(ServerUnit* attacker, int32 amount) {
    if (!m_inCombat) {
        m_inCombat = true;
    }
}

void ServerUnit::onHealReceived(ServerUnit* healer, int32 amount) {
    // Base implementation - nothing special
}
