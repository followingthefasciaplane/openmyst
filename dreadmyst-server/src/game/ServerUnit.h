#pragma once
#include "game/MutualObject.h"
#include "game/templates/SpellTemplate.h"
#include "game/templates/ItemTemplate.h"
#include <vector>
#include <map>
#include <memory>

class ServerMap;
class GamePacket;
#include "game/Spell.h"

// Aura tick data for one effect slot
struct AuraTickData {
    float  tickTotal         = 0.0f;
    float  tickAmount        = 0.0f;
    float  tickAmountTicked  = 0.0f;
    Guid   casterGuid        = 0;
    int32  numTicks          = 0;
    int32  numTicksCounter   = 0;
    int32  tickTimer         = 0;
    int32  tickIntervalMs    = 0;
};

// Active aura (buff/debuff) on a unit
struct Aura {
    uint32 spellId         = 0;
    Guid   casterGuid      = 0;
    int32  remainingMs     = 0;
    int32  stackCount      = 0;
    int32  spellLevel      = 0;
    int32  casterLevel     = 0;
    AuraTickData effects[MAX_AURA_EFFECTS] = {};

    bool isActive() const { return spellId != 0; }
};

// Base class for all combat-capable units (players and NPCs)
// Matches the original ServerUnit.cpp hierarchy
class ServerUnit : public MutualObject {
public:
    ~ServerUnit() override;

    // Identity
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    uint8  getClassId() const { return m_classId; }
    void   setClassId(uint8 id) { m_classId = id; }
    uint8  getGender() const { return m_gender; }
    void   setGender(uint8 g) { m_gender = g; }
    uint16 getLevel() const { return m_level; }
    void   setLevel(uint16 lvl) { m_level = lvl; }
    uint32 getFaction() const { return m_faction; }
    void   setFaction(uint32 f) { m_faction = f; }

    // Health/Mana
    int32  getHealth() const { return m_health; }
    int32  getMaxHealth() const { return m_maxHealth; }
    int32  getMana() const { return m_mana; }
    int32  getMaxMana() const { return m_maxMana; }
    float  getHealthPct() const { return m_maxHealth > 0 ? (float)m_health / m_maxHealth : 0.0f; }
    float  getManaPct() const { return m_maxMana > 0 ? (float)m_mana / m_maxMana : 0.0f; }
    bool   isAlive() const { return m_health > 0; }

    void setHealth(int32 hp);
    void setMana(int32 mp);
    void modifyHealth(int32 amount);
    void modifyMana(int32 amount);

    // Stats
    int32  getStat(StatType stat) const;
    void   setStat(StatType stat, int32 value);
    int32  getArmor() const { return m_armor; }
    void   setArmor(int32 armor) { m_armor = armor; }

    // Combat
    bool   isInCombat() const { return m_inCombat; }
    void   setInCombat(bool combat) { m_inCombat = combat; }
    Guid   getTargetGuid() const { return m_targetGuid; }
    void   setTarget(Guid guid) { m_targetGuid = guid; }
    float  getMoveSpeed() const { return m_moveSpeed; }
    void   setMoveSpeed(float speed) { m_moveSpeed = speed; }

    // Auras
    bool   addAura(const Aura& aura);
    void   removeAura(uint32 spellId);
    void   removeAllAuras();
    const std::vector<Aura>& getAuras() const { return m_auras; }
    bool   hasAura(uint32 spellId) const;
    void   tickAuras(int32 deltaMs);

    // Mechanics
    bool   hasMechanicImmunity(uint32 mechanic) const { return (m_mechanicImmuneMask & (1 << mechanic)) != 0; }
    uint32 getMechanicImmuneMask() const { return m_mechanicImmuneMask; }
    void   setMechanicImmuneMask(uint32 mask) { m_mechanicImmuneMask = mask; }

    // Map
    ServerMap* getMap() const { return m_map; }
    void setMap(ServerMap* map) { m_map = map; }

    // Current spell being cast
    Spell* getCurrentSpell() const { return m_currentSpell.get(); }
    void   setCurrentSpell(std::unique_ptr<Spell> spell) { m_currentSpell = std::move(spell); }
    void   clearCurrentSpell() { m_currentSpell.reset(); }
    bool   isCasting() const { return m_currentSpell != nullptr; }

    // Virtual methods for subclass behavior
    virtual void update(int32 deltaMs);
    virtual void onDeath(ServerUnit* killer);
    virtual void onDamageTaken(ServerUnit* attacker, int32 amount);
    virtual void onHealReceived(ServerUnit* healer, int32 amount);

protected:
    std::string m_name;
    uint8  m_classId     = 0;
    uint8  m_gender      = 0;
    uint16 m_level       = 1;
    uint32 m_faction     = 0;

    int32  m_health      = 0;
    int32  m_maxHealth    = 0;
    int32  m_mana        = 0;
    int32  m_maxMana      = 0;

    int32  m_stats[static_cast<int>(StatType::Count)] = {};
    int32  m_armor       = 0;

    bool   m_inCombat    = false;
    Guid   m_targetGuid  = 0;
    float  m_moveSpeed   = 1.0f;

    uint32 m_mechanicImmuneMask = 0;

    std::vector<Aura> m_auras;

    // Current spell being cast
    std::unique_ptr<Spell> m_currentSpell;

    ServerMap* m_map = nullptr;
};
