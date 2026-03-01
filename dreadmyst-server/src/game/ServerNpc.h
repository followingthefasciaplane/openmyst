#pragma once
#include "game/ServerUnit.h"
#include "game/templates/NpcTemplate.h"
#include <map>
#include <vector>
#include <memory>

class ServerPlayer;
class ServerMap;
class GameCache;

// ---- NPC AI base class and subclasses ----

// Base AI class for NPC behavior
class NpcAi {
public:
    virtual ~NpcAi() = default;

    virtual void updateAI(class ServerNpc* npc, int32 deltaMs) = 0;
    virtual void onEnterCombat(ServerNpc* npc, ServerUnit* target) {}
    virtual void onLeaveCombat(ServerNpc* npc) {}
    virtual void onDamageTaken(ServerNpc* npc, ServerUnit* attacker, int32 amount) {}
    virtual void selectTarget(ServerNpc* npc) {}
    virtual void castSpell(ServerNpc* npc, uint32 spellId, ServerUnit* target) {}
};

// Melee-focused AI: chases target, uses melee attacks
class NpcAiMelee : public NpcAi {
public:
    void updateAI(ServerNpc* npc, int32 deltaMs) override;
    void onEnterCombat(ServerNpc* npc, ServerUnit* target) override;
    void onLeaveCombat(ServerNpc* npc) override;
    void onDamageTaken(ServerNpc* npc, ServerUnit* attacker, int32 amount) override;
    void selectTarget(ServerNpc* npc) override;

private:
    int32 m_autoAttackTimer = 0;
};

// Caster/Archer AI: maintains distance, prefers ranged spells
class NpcAiCasterArcher : public NpcAi {
public:
    void updateAI(ServerNpc* npc, int32 deltaMs) override;
    void onEnterCombat(ServerNpc* npc, ServerUnit* target) override;
    void onLeaveCombat(ServerNpc* npc) override;
    void onDamageTaken(ServerNpc* npc, ServerUnit* attacker, int32 amount) override;
    void selectTarget(ServerNpc* npc) override;
    void castSpell(ServerNpc* npc, uint32 spellId, ServerUnit* target) override;

private:
    int32 m_spellCooldowns[MAX_NPC_SPELLS] = {};
    int32 m_autoAttackTimer = 0;
};

// ---- Movement generators ----

// Base class for NPC movement behavior
class UnitMotionGen {
public:
    virtual ~UnitMotionGen() = default;

    virtual void update(ServerNpc* npc, int32 deltaMs) = 0;
    virtual void start(ServerNpc* npc) {}
    virtual void stop(ServerNpc* npc) {}
    virtual MovementType getType() const = 0;
};

// Idle: stand in place
class IdleMotionGen : public UnitMotionGen {
public:
    void update(ServerNpc* npc, int32 deltaMs) override;
    MovementType getType() const override { return MovementType::Idle; }
};

// Random wander within distance of home position
class RandomMotionGen : public UnitMotionGen {
public:
    explicit RandomMotionGen(float wanderDistance);
    void update(ServerNpc* npc, int32 deltaMs) override;
    void start(ServerNpc* npc) override;
    MovementType getType() const override { return MovementType::Random; }

private:
    float m_wanderDistance = 0.0f;
    float m_targetX       = 0.0f;
    float m_targetY       = 0.0f;
    int32 m_waitTimer     = 0;
    bool  m_moving        = false;
};

// Patrol along waypoints
class PatrolMotionGen : public UnitMotionGen {
public:
    explicit PatrolMotionGen(const std::vector<NpcWaypoint>& waypoints);
    void update(ServerNpc* npc, int32 deltaMs) override;
    void start(ServerNpc* npc) override;
    MovementType getType() const override { return MovementType::Patrol; }

private:
    std::vector<NpcWaypoint> m_waypoints;
    uint32 m_currentPoint = 0;
    int32  m_waitTimer    = 0;
    bool   m_forward      = true;  // ping-pong direction
};

// Chase a target unit
class ChaseMotionGen : public UnitMotionGen {
public:
    explicit ChaseMotionGen(Guid targetGuid);
    void update(ServerNpc* npc, int32 deltaMs) override;
    void start(ServerNpc* npc) override;
    MovementType getType() const override { return MovementType::Chase; }

private:
    Guid  m_targetGuid = 0;
    int32 m_updateTimer = 0;
};

// Return to home (spawn) position
class HomeMotionGen : public UnitMotionGen {
public:
    void update(ServerNpc* npc, int32 deltaMs) override;
    void start(ServerNpc* npc) override;
    MovementType getType() const override { return MovementType::Home; }
};

// ---- NPC death state ----

enum class NpcDeathState : uint8 {
    Alive   = 0,
    Dead    = 1,
    Corpse  = 2
};

// ---- ServerNpc: NPC entity matching ServerNpc.cpp from the original binary ----

class ServerNpc : public ServerUnit {
public:
    ServerNpc(const NpcTemplate& tmpl, const NpcSpawn& spawn);
    ~ServerNpc() override;

    // -- Template & spawn info --
    uint32 getSpawnGuid() const { return m_spawnGuid; }
    uint32 getTemplateEntry() const { return m_templateEntry; }
    const NpcTemplate& getTemplate() const { return m_template; }

    // Spawn (home) position
    float  getSpawnX() const { return m_spawnX; }
    float  getSpawnY() const { return m_spawnY; }
    float  getSpawnOrientation() const { return m_spawnOrientation; }

    // -- Gossip --
    uint32 getGossipMenuId() const { return m_template.gossipMenuId; }

    // -- NPC flags --
    uint32 getNpcFlags() const { return m_template.npcFlags; }
    bool   hasNpcFlag(uint32 flag) const { return (m_template.npcFlags & flag) != 0; }
    bool   isGossip() const { return hasNpcFlag(NPC_FLAG_GOSSIP); }
    bool   isVendor() const { return hasNpcFlag(NPC_FLAG_VENDOR); }
    bool   isQuestGiver() const { return hasNpcFlag(NPC_FLAG_QUEST); }
    bool   isRepair() const { return hasNpcFlag(NPC_FLAG_REPAIR); }
    bool   isBanker() const { return hasNpcFlag(NPC_FLAG_BANKER); }
    bool   isInnkeeper() const { return hasNpcFlag(NPC_FLAG_INNKEEPER); }

    // Boss/Elite
    bool   isElite() const { return m_template.boolElite != 0; }
    bool   isBoss() const { return m_template.boolBoss != 0; }

    // -- Faction --
    uint32 getFactionId() const { return m_template.faction; }

    // -- Leash range --
    float  getLeashRange() const { return m_template.leashRange; }
    bool   isInLeashRange() const;

    // -- Death / Respawn --
    NpcDeathState getDeathState() const { return m_deathState; }
    void   setDeathState(NpcDeathState state) { m_deathState = state; }
    int32  getRespawnTime() const { return m_respawnTimeMs; }
    bool   isRespawning() const { return m_deathState != NpcDeathState::Alive; }

    // -- Call for help --
    bool   canCallForHelp() const { return m_callForHelp; }
    void   callForHelp(float radius);

    // -- Threat system --
    void   addThreat(Guid guid, float amount);
    void   removeThreat(Guid guid);
    void   clearThreatList();
    float  getThreat(Guid guid) const;
    Guid   getHighestThreatTarget() const;
    bool   hasThreat(Guid guid) const;
    const std::map<Guid, float>& getThreatList() const { return m_threatList; }

    // -- Combat AI --
    NpcAi* getAi() const { return m_ai.get(); }
    void   selectTarget();
    void   updateAI(int32 deltaMs);

    // -- Movement --
    UnitMotionGen* getMotionGen() const { return m_motionGen.get(); }
    void   setMotionGen(std::unique_ptr<UnitMotionGen> gen);
    void   returnToHome();

    // -- Loot --
    void   generateLoot();
    const std::vector<ItemInstance>& getLoot() const { return m_loot; }
    void   clearLoot() { m_loot.clear(); }

    // -- Main update tick --
    void   update(int32 deltaMs) override;

    // -- Death/damage callbacks --
    void   onDeath(ServerUnit* killer) override;
    void   onDamageTaken(ServerUnit* attacker, int32 amount) override;

    // -- Respawn --
    void   respawn();

    // -- GameCache reference (set by ServerMap on spawn) --
    void   setGameCache(const GameCache* cache) { m_gameCache = cache; }
    const GameCache* getGameCache() const { return m_gameCache; }

private:
    void initFromTemplate(const NpcTemplate& tmpl, const NpcSpawn& spawn);
    void createAi(AiType aiType);
    void createDefaultMotionGen(const NpcSpawn& spawn);
    void updateCombat(int32 deltaMs);
    void updateRespawn(int32 deltaMs);

    // Template data (cached copy)
    NpcTemplate m_template;
    uint32      m_spawnGuid         = 0;
    uint32      m_templateEntry     = 0;

    // Spawn (home) position
    float  m_spawnX           = 0.0f;
    float  m_spawnY           = 0.0f;
    float  m_spawnOrientation = 0.0f;

    // Death & respawn
    NpcDeathState m_deathState    = NpcDeathState::Alive;
    int32         m_respawnTimeMs = 0;
    int32         m_respawnTimer  = 0;

    // Call for help
    bool m_callForHelp = false;

    // Threat list: target Guid -> accumulated threat value
    std::map<Guid, float> m_threatList;

    // AI
    std::unique_ptr<NpcAi> m_ai;

    // Movement
    std::unique_ptr<UnitMotionGen> m_motionGen;

    // Loot
    std::vector<ItemInstance> m_loot;

    // Spell cooldown timers (per slot)
    int32 m_spellCooldownTimers[MAX_NPC_SPELLS] = {};

    // Cache reference
    const GameCache* m_gameCache = nullptr;
};
