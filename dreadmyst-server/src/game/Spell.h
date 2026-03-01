#pragma once
#include "core/Types.h"
#include "game/templates/SpellTemplate.h"
#include <memory>
#include <vector>

class ServerUnit;
class ServerMap;

// Spell cast result codes
enum class SpellCastResult : uint32 {
    Success         = 0,
    NotReady        = 1,
    OutOfRange      = 2,
    NotEnoughMana   = 3,
    InvalidTarget   = 4,
    OnCooldown      = 5,
    CantCastWhileMoving = 6,
    Interrupted     = 7,
    LineOfSight     = 8,
    TargetDead      = 9,
    CasterDead      = 10,
    SpellNotKnown   = 11,
    FailedRequirement = 12
};

// Active spell being cast
class Spell {
public:
    Spell(ServerUnit* caster, const SpellTemplate* tmpl);
    ~Spell() = default;

    // Cast parameters
    void setTarget(Guid targetGuid) { m_targetGuid = targetGuid; }
    void setTargetPos(float x, float y) { m_targetX = x; m_targetY = y; }
    void setSpellLevel(int32 level) { m_spellLevel = level; }
    void setIsAutoRepeat(bool v) { m_isAutoRepeat = v; }

    // Validation
    SpellCastResult validate();
    SpellCastResult checkRange();
    SpellCastResult checkMana();
    SpellCastResult checkCooldown();

    // Execution
    void prepare();        // Start cast time
    void cast();           // Execute effects
    void finish();         // Cleanup after cast
    void cancel();         // Cancel mid-cast
    void update(int32 deltaMs); // Tick cast timer

    // Getters
    ServerUnit* getCaster() const { return m_caster; }
    const SpellTemplate* getTemplate() const { return m_template; }
    Guid getTargetGuid() const { return m_targetGuid; }
    bool isCasting() const { return m_isCasting; }
    bool isFinished() const { return m_finished; }

private:
    void executeEffect(int effectIndex, ServerUnit* target);
    void applyAura(int effectIndex, ServerUnit* target);
    std::vector<ServerUnit*> resolveTargets(int effectIndex);
    void sendSpellGo();
    void sendCastResult(SpellCastResult result);
    void consumeMana();
    void triggerCooldown();

    ServerUnit*          m_caster    = nullptr;
    const SpellTemplate* m_template  = nullptr;
    Guid                 m_targetGuid = 0;
    float                m_targetX   = 0.0f;
    float                m_targetY   = 0.0f;
    int32                m_spellLevel = 1;
    int32                m_castTimer = 0;
    bool                 m_isCasting = false;
    bool                 m_finished  = false;
    bool                 m_isAutoRepeat = false;
};
