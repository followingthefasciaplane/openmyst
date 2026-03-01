#pragma once

#include <cstdint>
#include <map>
#include <ctime>

// Tracks spell/ability cooldowns with expiry timestamps
class CooldownHolder
{
public:
    void addCooldown(int spellId, int durationMs);
    void removeCooldown(int spellId);
    bool isOnCooldown(int spellId) const;
    int getRemainingMs(int spellId) const;
    void clear();

    const std::map<int, clock_t>& getCooldowns() const { return m_cooldowns; }

private:
    // Map of spellId -> expiry time (clock_t)
    std::map<int, clock_t> m_cooldowns;
};
