#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "GameDefines.h"
#include "render/ModelScript.h"

// WorldEntity - a single entity in the game world (player, NPC, or game object).
// Holds position, identity, stats, movement interpolation, and animation state.
struct WorldEntity
{
    uint32_t guid = 0;
    ObjDefines::ObjectType type = ObjDefines::Type_None;
    std::string name;
    std::string subname;

    // Position (grid coordinates)
    float posX = 0.f, posY = 0.f;
    float orientation = 0.f;

    // Identity
    uint8_t  classId  = 0;
    uint8_t  gender   = 0;
    uint8_t  portrait = 0;
    uint16_t level    = 0;

    // Stats
    int32_t health    = 0, maxHealth = 0;
    int32_t mana      = 0, maxMana   = 0;
    float   moveSpeed = 1.0f;

    // NPC-specific
    uint32_t entryId = 0;

    // Object variables (indexed by ObjDefines::Variable)
    std::vector<int32_t> variables;

    // Movement interpolation
    float targetX = 0.f, targetY = 0.f;
    bool  isMoving = false;

    // Spline movement
    struct Waypoint { float x, y; };
    std::vector<Waypoint> splineWaypoints;
    int splineIndex = 0;
    float splineStartX = 0.f, splineStartY = 0.f;
    bool splineActive = false;

    // Sprite/animation state
    ModelAnimator animator;
    std::string modelName;     // resolved sprite model name
    float modelScale = 1.0f;   // rendering scale from npc_template
    bool  modelResolved = false; // true once modelName has been looked up

    int32_t getVariable(int varId) const
    {
        if (varId >= 0 && varId < static_cast<int>(variables.size()))
            return variables[varId];
        return 0;
    }

    void setVariable(int varId, int32_t value)
    {
        if (varId < 0) return;
        if (varId >= static_cast<int>(variables.size()))
            variables.resize(static_cast<size_t>(varId) + 1, 0);
        variables[varId] = value;
    }
};

// WorldState - singleton registry of all world entities by GUID.
class WorldState
{
public:
    static WorldState& instance();

    // Add or update an entity
    WorldEntity& addEntity(uint32_t guid);

    // Remove an entity
    void removeEntity(uint32_t guid);

    // Get entity by GUID (nullptr if not found)
    WorldEntity* getEntity(uint32_t guid);
    const WorldEntity* getEntity(uint32_t guid) const;

    // All entities (for rendering + animation updates)
    const std::unordered_map<uint32_t, WorldEntity>& getAllEntities() const { return m_entities; }
    std::unordered_map<uint32_t, WorldEntity>& getAllEntities() { return m_entities; }

    // Clear all entities (e.g., on map change)
    void clear();

    // Update movement interpolation for all entities
    void update(float dt);

private:
    WorldState() = default;
    std::unordered_map<uint32_t, WorldEntity> m_entities;
};

#define sWorldState WorldState::instance()
