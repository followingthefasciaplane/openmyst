#include "game/ServerNpc.h"
#include "game/GameCache.h"
#include "core/Log.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ============================================================================
// ServerNpc
// ============================================================================

ServerNpc::ServerNpc(const NpcTemplate& tmpl, const NpcSpawn& spawn) {
    initFromTemplate(tmpl, spawn);
}

ServerNpc::~ServerNpc() = default;

void ServerNpc::initFromTemplate(const NpcTemplate& tmpl, const NpcSpawn& spawn) {
    m_template      = tmpl;
    m_spawnGuid     = spawn.guid;
    m_templateEntry = tmpl.entry;

    // MutualObject fields
    setGuid(spawn.guid);
    setEntry(tmpl.entry);
    setMapId(spawn.map);
    setPosition(spawn.positionX, spawn.positionY);
    setOrientation(spawn.orientation);

    // Spawn (home) position
    m_spawnX           = spawn.positionX;
    m_spawnY           = spawn.positionY;
    m_spawnOrientation = spawn.orientation;

    // ServerUnit fields
    setName(tmpl.name);
    m_faction = tmpl.faction;
    m_armor   = static_cast<int32>(tmpl.armor);

    // Level: random between minLevel and maxLevel
    if (tmpl.maxLevel > tmpl.minLevel) {
        m_level = static_cast<uint16>(tmpl.minLevel + (std::rand() % (tmpl.maxLevel - tmpl.minLevel + 1)));
    } else {
        m_level = static_cast<uint16>(tmpl.minLevel);
    }

    // Health and mana from template
    m_maxHealth = static_cast<int32>(tmpl.health);
    m_health    = m_maxHealth;
    m_maxMana   = static_cast<int32>(tmpl.mana);
    m_mana      = m_maxMana;

    // Stats from template
    setStat(StatType::Strength,     static_cast<int32>(tmpl.strength));
    setStat(StatType::Agility,      static_cast<int32>(tmpl.agility));
    setStat(StatType::Intelligence, static_cast<int32>(tmpl.intellect));
    setStat(StatType::Willpower,    static_cast<int32>(tmpl.willpower));
    setStat(StatType::Courage,      static_cast<int32>(tmpl.courage));

    // Mechanic immunities
    setMechanicImmuneMask(tmpl.mechanicImmuneMask);

    // Death state from spawn data
    m_deathState  = static_cast<NpcDeathState>(spawn.deathState);
    m_callForHelp = (spawn.callForHelp != 0);

    // Respawn time from spawn data (seconds -> milliseconds)
    m_respawnTimeMs = static_cast<int32>(spawn.respawnTime) * 1000;
    m_respawnTimer  = 0;

    // Create AI based on template
    createAi(static_cast<AiType>(tmpl.aiType));

    // Create default movement generator based on spawn/template
    createDefaultMotionGen(spawn);

    LOG_DEBUG("ServerNpc::init entry=%u guid=%u name='%s' level=%u hp=%d",
              tmpl.entry, spawn.guid, tmpl.name.c_str(), m_level, m_health);
}

void ServerNpc::createAi(AiType aiType) {
    switch (aiType) {
        case AiType::CasterArcher:
            m_ai = std::make_unique<NpcAiCasterArcher>();
            break;
        case AiType::Melee:
        default:
            m_ai = std::make_unique<NpcAiMelee>();
            break;
    }
}

void ServerNpc::createDefaultMotionGen(const NpcSpawn& spawn) {
    MovementType movType = static_cast<MovementType>(spawn.movementType);
    switch (movType) {
        case MovementType::Random:
            m_motionGen = std::make_unique<RandomMotionGen>(spawn.wanderDistance);
            break;
        case MovementType::Patrol:
            // Patrol waypoints will be set later by ServerMap from GameCache
            m_motionGen = std::make_unique<PatrolMotionGen>(std::vector<NpcWaypoint>{});
            break;
        case MovementType::Idle:
        default:
            m_motionGen = std::make_unique<IdleMotionGen>();
            break;
    }
    if (m_motionGen) {
        m_motionGen->start(this);
    }
}

// -- Leash range --

bool ServerNpc::isInLeashRange() const {
    if (m_template.leashRange <= 0.0f) {
        return true;  // no leash configured
    }
    float dist = distanceTo(m_spawnX, m_spawnY);
    return dist <= m_template.leashRange;
}

// -- Call for help --

void ServerNpc::callForHelp(float radius) {
    // Notify nearby friendly NPCs to assist
    // Implementation depends on ServerMap neighbor queries
    LOG_DEBUG("ServerNpc::callForHelp guid=%u radius=%.1f", m_spawnGuid, radius);
}

// -- Threat system --

void ServerNpc::addThreat(Guid guid, float amount) {
    m_threatList[guid] += amount;
}

void ServerNpc::removeThreat(Guid guid) {
    m_threatList.erase(guid);
}

void ServerNpc::clearThreatList() {
    m_threatList.clear();
}

float ServerNpc::getThreat(Guid guid) const {
    auto it = m_threatList.find(guid);
    return (it != m_threatList.end()) ? it->second : 0.0f;
}

Guid ServerNpc::getHighestThreatTarget() const {
    Guid bestTarget = 0;
    float bestThreat = 0.0f;
    for (const auto& [guid, threat] : m_threatList) {
        if (threat > bestThreat) {
            bestThreat = threat;
            bestTarget = guid;
        }
    }
    return bestTarget;
}

bool ServerNpc::hasThreat(Guid guid) const {
    return m_threatList.find(guid) != m_threatList.end();
}

// -- Combat AI --

void ServerNpc::selectTarget() {
    if (m_ai) {
        m_ai->selectTarget(this);
    }
}

void ServerNpc::updateAI(int32 deltaMs) {
    if (m_ai) {
        m_ai->updateAI(this, deltaMs);
    }
}

// -- Movement --

void ServerNpc::setMotionGen(std::unique_ptr<UnitMotionGen> gen) {
    if (m_motionGen) {
        m_motionGen->stop(this);
    }
    m_motionGen = std::move(gen);
    if (m_motionGen) {
        m_motionGen->start(this);
    }
}

void ServerNpc::returnToHome() {
    setMotionGen(std::make_unique<HomeMotionGen>());
}

// -- Loot generation --

void ServerNpc::generateLoot() {
    m_loot.clear();

    if (!m_gameCache) return;

    // Custom loot table
    if (m_template.customLoot != 0) {
        const auto& lootEntries = m_gameCache->getLootEntries(m_template.customLoot);
        for (const auto& entry : lootEntries) {
            float roll = static_cast<float>(std::rand() % 10000) / 100.0f;
            if (roll < entry.chance) {
                uint32 count = entry.countMin;
                if (entry.countMax > entry.countMin) {
                    count += std::rand() % (entry.countMax - entry.countMin + 1);
                }
                const ItemTemplate* itemTmpl = m_gameCache->getItemTemplate(entry.item);
                if (itemTmpl) {
                    ItemInstance inst;
                    inst.entry = itemTmpl->entry;
                    inst.count = count;
                    m_loot.push_back(inst);
                }
            }
        }
    }

    LOG_DEBUG("ServerNpc::generateLoot guid=%u items=%zu", m_spawnGuid, m_loot.size());
}

// -- Main update --

void ServerNpc::update(int32 deltaMs) {
    // Base class: tick auras
    ServerUnit::update(deltaMs);

    // Dead NPCs only tick respawn timer
    if (m_deathState != NpcDeathState::Alive) {
        updateRespawn(deltaMs);
        return;
    }

    // AI tick
    updateAI(deltaMs);

    // Combat tick
    if (isInCombat()) {
        updateCombat(deltaMs);
    }

    // Movement tick
    if (m_motionGen) {
        m_motionGen->update(this, deltaMs);
    }

    // Leash check: if too far from home, evade and return
    if (isInCombat() && !isInLeashRange()) {
        LOG_DEBUG("ServerNpc::update guid=%u exceeded leash range, returning home", m_spawnGuid);
        setInCombat(false);
        clearThreatList();
        setTarget(0);
        removeAllAuras();
        // Restore full health on leash evade
        setHealth(getMaxHealth());
        setMana(getMaxMana());
        returnToHome();
    }
}

void ServerNpc::updateCombat(int32 deltaMs) {
    // Update spell cooldowns
    for (int i = 0; i < MAX_NPC_SPELLS; i++) {
        if (m_spellCooldownTimers[i] > 0) {
            m_spellCooldownTimers[i] -= deltaMs;
            if (m_spellCooldownTimers[i] < 0) {
                m_spellCooldownTimers[i] = 0;
            }
        }
    }

    // Check if current target is still valid
    Guid targetGuid = getTargetGuid();
    if (targetGuid == 0 || !hasThreat(targetGuid)) {
        // Select a new target from threat list
        Guid newTarget = getHighestThreatTarget();
        if (newTarget != 0) {
            setTarget(newTarget);
        } else {
            // No targets left, leave combat
            setInCombat(false);
            clearThreatList();
            setTarget(0);
            returnToHome();
        }
    }
}

void ServerNpc::updateRespawn(int32 deltaMs) {
    if (m_respawnTimeMs <= 0) return;  // no respawn configured

    m_respawnTimer += deltaMs;
    if (m_respawnTimer >= m_respawnTimeMs) {
        respawn();
    }
}

// -- Death --

void ServerNpc::onDeath(ServerUnit* killer) {
    ServerUnit::onDeath(killer);

    m_deathState   = NpcDeathState::Dead;
    m_respawnTimer = 0;

    setInCombat(false);
    clearThreatList();
    setTarget(0);

    // Generate loot on death
    generateLoot();

    // Stop movement
    if (m_motionGen) {
        m_motionGen->stop(this);
    }

    LOG_INFO("ServerNpc::onDeath guid=%u entry=%u name='%s'",
             m_spawnGuid, m_templateEntry, m_name.c_str());
}

void ServerNpc::onDamageTaken(ServerUnit* attacker, int32 amount) {
    ServerUnit::onDamageTaken(attacker, amount);

    if (attacker) {
        addThreat(attacker->getGuid(), static_cast<float>(amount));

        // Enter combat if not already
        if (!isInCombat()) {
            setInCombat(true);
            if (m_ai) {
                m_ai->onEnterCombat(this, attacker);
            }
            // Switch to chase movement
            setMotionGen(std::make_unique<ChaseMotionGen>(attacker->getGuid()));
        }

        // Call for help on first hit if enabled
        if (m_callForHelp && m_threatList.size() == 1) {
            callForHelp(30.0f);
        }

        // Notify AI
        if (m_ai) {
            m_ai->onDamageTaken(this, attacker, amount);
        }
    }
}

// -- Respawn --

void ServerNpc::respawn() {
    m_deathState   = NpcDeathState::Alive;
    m_respawnTimer = 0;

    // Restore to spawn position
    setPosition(m_spawnX, m_spawnY);
    setOrientation(m_spawnOrientation);

    // Restore full health/mana
    m_health = m_maxHealth;
    m_mana   = m_maxMana;

    // Clear loot
    clearLoot();

    // Re-create default movement
    NpcSpawn fakeSpawn;
    fakeSpawn.movementType   = m_template.movementType;
    fakeSpawn.wanderDistance  = 0.0f;  // will be set from template if needed
    createDefaultMotionGen(fakeSpawn);

    LOG_INFO("ServerNpc::respawn guid=%u entry=%u name='%s'",
             m_spawnGuid, m_templateEntry, m_name.c_str());
}

// ============================================================================
// NpcAiMelee
// ============================================================================

void NpcAiMelee::updateAI(ServerNpc* npc, int32 deltaMs) {
    if (!npc->isInCombat() || !npc->isAlive()) return;

    // Auto-attack timer
    const NpcTemplate& tmpl = npc->getTemplate();
    if (tmpl.meleeSpeed > 0) {
        m_autoAttackTimer -= deltaMs;
        if (m_autoAttackTimer <= 0) {
            m_autoAttackTimer = static_cast<int32>(tmpl.meleeSpeed);
            // Melee attack damage based on weaponValue
            // Actual damage application handled by combat system
        }
    }

    // Try to use spells (melee AI uses spells occasionally)
    for (int i = 0; i < MAX_NPC_SPELLS; i++) {
        const NpcSpellSlot& slot = tmpl.spells[i];
        if (slot.spellId == 0) continue;

        // Roll chance
        uint32 roll = std::rand() % 100;
        if (roll < slot.chance) {
            // Spell usage deferred to spell system
        }
    }
}

void NpcAiMelee::onEnterCombat(ServerNpc* npc, ServerUnit* target) {
    npc->setTarget(target->getGuid());
    m_autoAttackTimer = 0;
}

void NpcAiMelee::onLeaveCombat(ServerNpc* npc) {
    m_autoAttackTimer = 0;
}

void NpcAiMelee::onDamageTaken(ServerNpc* npc, ServerUnit* attacker, int32 amount) {
    // Re-evaluate target if attacker has higher threat
    selectTarget(npc);
}

void NpcAiMelee::selectTarget(ServerNpc* npc) {
    Guid highestThreat = npc->getHighestThreatTarget();
    if (highestThreat != 0) {
        npc->setTarget(highestThreat);
    }
}

// ============================================================================
// NpcAiCasterArcher
// ============================================================================

void NpcAiCasterArcher::updateAI(ServerNpc* npc, int32 deltaMs) {
    if (!npc->isInCombat() || !npc->isAlive()) return;

    const NpcTemplate& tmpl = npc->getTemplate();

    // Update spell cooldowns
    for (int i = 0; i < MAX_NPC_SPELLS; i++) {
        if (m_spellCooldowns[i] > 0) {
            m_spellCooldowns[i] -= deltaMs;
        }
    }

    // Try to cast spells in priority order
    for (int i = 0; i < MAX_NPC_SPELLS; i++) {
        const NpcSpellSlot& slot = tmpl.spells[i];
        if (slot.spellId == 0) continue;
        if (m_spellCooldowns[i] > 0) continue;

        uint32 roll = std::rand() % 100;
        if (roll < slot.chance) {
            m_spellCooldowns[i] = static_cast<int32>(slot.cooldown);
            // Cast deferred to spell system
            break;  // one spell per tick
        }
    }

    // Fall back to ranged auto-attack
    if (tmpl.rangedSpeed > 0) {
        m_autoAttackTimer -= deltaMs;
        if (m_autoAttackTimer <= 0) {
            m_autoAttackTimer = static_cast<int32>(tmpl.rangedSpeed);
            // Ranged attack based on rangedWeaponValue
        }
    }
}

void NpcAiCasterArcher::onEnterCombat(ServerNpc* npc, ServerUnit* target) {
    npc->setTarget(target->getGuid());
    m_autoAttackTimer = 0;
    for (int i = 0; i < MAX_NPC_SPELLS; i++) {
        m_spellCooldowns[i] = 0;
    }
}

void NpcAiCasterArcher::onLeaveCombat(ServerNpc* npc) {
    m_autoAttackTimer = 0;
}

void NpcAiCasterArcher::onDamageTaken(ServerNpc* npc, ServerUnit* attacker, int32 amount) {
    selectTarget(npc);
}

void NpcAiCasterArcher::selectTarget(ServerNpc* npc) {
    Guid highestThreat = npc->getHighestThreatTarget();
    if (highestThreat != 0) {
        npc->setTarget(highestThreat);
    }
}

void NpcAiCasterArcher::castSpell(ServerNpc* npc, uint32 spellId, ServerUnit* target) {
    // Spell casting handled by the spell system
    LOG_DEBUG("NpcAiCasterArcher::castSpell npc=%u spell=%u target=%u",
              npc->getSpawnGuid(), spellId, target ? target->getGuid() : 0);
}

// ============================================================================
// Movement Generators
// ============================================================================

// -- IdleMotionGen --

void IdleMotionGen::update(ServerNpc* /*npc*/, int32 /*deltaMs*/) {
    // Stand still, do nothing
}

// -- RandomMotionGen --

RandomMotionGen::RandomMotionGen(float wanderDistance)
    : m_wanderDistance(wanderDistance) {
}

void RandomMotionGen::start(ServerNpc* npc) {
    m_moving    = false;
    m_waitTimer = 2000 + (std::rand() % 5000);  // 2-7 second initial wait
}

void RandomMotionGen::update(ServerNpc* npc, int32 deltaMs) {
    if (npc->isInCombat()) return;

    if (!m_moving) {
        // Waiting between movements
        m_waitTimer -= deltaMs;
        if (m_waitTimer <= 0) {
            // Pick random point within wander distance of home
            float angle = static_cast<float>(std::rand() % 360) * 3.14159f / 180.0f;
            float dist  = static_cast<float>(std::rand() % static_cast<int>(m_wanderDistance * 100)) / 100.0f;
            m_targetX = npc->getSpawnX() + std::cos(angle) * dist;
            m_targetY = npc->getSpawnY() + std::sin(angle) * dist;
            m_moving  = true;
        }
        return;
    }

    // Move toward target point
    float dx = m_targetX - npc->getPositionX();
    float dy = m_targetY - npc->getPositionY();
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 0.5f) {
        // Arrived at destination
        npc->setPosition(m_targetX, m_targetY);
        m_moving    = false;
        m_waitTimer = 3000 + (std::rand() % 6000);  // 3-9 second wait
        return;
    }

    // Step toward target
    float speed     = npc->getMoveSpeed();
    float stepDist  = speed * (static_cast<float>(deltaMs) / 1000.0f);
    if (stepDist > dist) stepDist = dist;

    float nx = dx / dist;
    float ny = dy / dist;
    npc->setPosition(npc->getPositionX() + nx * stepDist,
                     npc->getPositionY() + ny * stepDist);
    npc->setOrientation(std::atan2(ny, nx));
}

// -- PatrolMotionGen --

PatrolMotionGen::PatrolMotionGen(const std::vector<NpcWaypoint>& waypoints)
    : m_waypoints(waypoints) {
}

void PatrolMotionGen::start(ServerNpc* npc) {
    m_currentPoint = 0;
    m_waitTimer    = 0;
    m_forward      = true;
}

void PatrolMotionGen::update(ServerNpc* npc, int32 deltaMs) {
    if (npc->isInCombat()) return;
    if (m_waypoints.empty()) return;

    // Wait at current waypoint
    if (m_waitTimer > 0) {
        m_waitTimer -= deltaMs;
        return;
    }

    const NpcWaypoint& wp = m_waypoints[m_currentPoint];

    // Move toward current waypoint
    float dx   = wp.positionX - npc->getPositionX();
    float dy   = wp.positionY - npc->getPositionY();
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 0.5f) {
        // Arrived at waypoint
        npc->setPosition(wp.positionX, wp.positionY);
        npc->setOrientation(wp.orientation);
        m_waitTimer = static_cast<int32>(wp.waitTime);

        // Advance to next waypoint (ping-pong)
        if (m_forward) {
            if (m_currentPoint + 1 < static_cast<uint32>(m_waypoints.size())) {
                m_currentPoint++;
            } else {
                m_forward = false;
                if (m_currentPoint > 0) m_currentPoint--;
            }
        } else {
            if (m_currentPoint > 0) {
                m_currentPoint--;
            } else {
                m_forward = true;
                if (m_waypoints.size() > 1) m_currentPoint++;
            }
        }
        return;
    }

    // Step toward waypoint
    float speedMult = wp.run ? 2.0f : 1.0f;
    float speed     = npc->getMoveSpeed() * speedMult;
    float stepDist  = speed * (static_cast<float>(deltaMs) / 1000.0f);
    if (stepDist > dist) stepDist = dist;

    float nx = dx / dist;
    float ny = dy / dist;
    npc->setPosition(npc->getPositionX() + nx * stepDist,
                     npc->getPositionY() + ny * stepDist);
    npc->setOrientation(std::atan2(ny, nx));
}

// -- ChaseMotionGen --

ChaseMotionGen::ChaseMotionGen(Guid targetGuid)
    : m_targetGuid(targetGuid) {
}

void ChaseMotionGen::start(ServerNpc* /*npc*/) {
    m_updateTimer = 0;
}

void ChaseMotionGen::update(ServerNpc* npc, int32 deltaMs) {
    if (!npc->isInCombat()) return;

    // Chase target position is resolved by ServerMap (looks up target unit)
    // Movement toward target handled here at a basic level
    // Full implementation requires ServerMap::getUnit(m_targetGuid) lookup

    m_updateTimer -= deltaMs;
    if (m_updateTimer > 0) return;
    m_updateTimer = 250;  // recalculate path every 250ms
}

// -- HomeMotionGen --

void HomeMotionGen::start(ServerNpc* npc) {
    // Begin returning home
    LOG_DEBUG("HomeMotionGen::start guid=%u returning to (%.1f, %.1f)",
              npc->getSpawnGuid(), npc->getSpawnX(), npc->getSpawnY());
}

void HomeMotionGen::update(ServerNpc* npc, int32 deltaMs) {
    float dx   = npc->getSpawnX() - npc->getPositionX();
    float dy   = npc->getSpawnY() - npc->getPositionY();
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 0.5f) {
        // Arrived home
        npc->setPosition(npc->getSpawnX(), npc->getSpawnY());
        npc->setOrientation(npc->getSpawnOrientation());

        // Restore health to full after evade
        npc->setHealth(npc->getMaxHealth());
        npc->setMana(npc->getMaxMana());

        // Switch back to default idle movement
        npc->setMotionGen(std::make_unique<IdleMotionGen>());
        return;
    }

    // Move toward home at increased speed (evade speed)
    float speed    = npc->getMoveSpeed() * 2.0f;
    float stepDist = speed * (static_cast<float>(deltaMs) / 1000.0f);
    if (stepDist > dist) stepDist = dist;

    float nx = dx / dist;
    float ny = dy / dist;
    npc->setPosition(npc->getPositionX() + nx * stepDist,
                     npc->getPositionY() + ny * stepDist);
    npc->setOrientation(std::atan2(ny, nx));
}
