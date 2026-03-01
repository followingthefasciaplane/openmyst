#include "WorldState.h"
#include <cmath>

WorldState& WorldState::instance()
{
    static WorldState inst;
    return inst;
}

WorldEntity& WorldState::addEntity(uint32_t guid)
{
    auto& ent = m_entities[guid];
    ent.guid = guid;
    return ent;
}

void WorldState::removeEntity(uint32_t guid)
{
    m_entities.erase(guid);
}

WorldEntity* WorldState::getEntity(uint32_t guid)
{
    auto it = m_entities.find(guid);
    return (it != m_entities.end()) ? &it->second : nullptr;
}

const WorldEntity* WorldState::getEntity(uint32_t guid) const
{
    auto it = m_entities.find(guid);
    return (it != m_entities.end()) ? &it->second : nullptr;
}

void WorldState::clear()
{
    m_entities.clear();
}

void WorldState::update(float dt)
{
    for (auto& [guid, ent] : m_entities)
    {
        // Spline movement takes priority
        if (ent.splineActive && !ent.splineWaypoints.empty())
        {
            if (ent.splineIndex < static_cast<int>(ent.splineWaypoints.size()))
            {
                auto& wp = ent.splineWaypoints[ent.splineIndex];
                float dx = wp.x - ent.posX;
                float dy = wp.y - ent.posY;
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist > 0.1f)
                {
                    float step = ent.moveSpeed * dt;
                    if (step >= dist)
                    {
                        ent.posX = wp.x;
                        ent.posY = wp.y;
                        ent.splineIndex++;
                    }
                    else
                    {
                        float ratio = step / dist;
                        ent.posX += dx * ratio;
                        ent.posY += dy * ratio;
                    }
                    ent.isMoving = true;
                }
                else
                {
                    ent.splineIndex++;
                }
            }
            else
            {
                // Spline complete
                ent.splineActive = false;
                ent.isMoving = false;
            }
            continue;
        }

        // Target-based interpolation (from position broadcasts)
        float dx = ent.targetX - ent.posX;
        float dy = ent.targetY - ent.posY;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 0.1f)
        {
            float step = ent.moveSpeed * dt;
            if (step >= dist)
            {
                ent.posX = ent.targetX;
                ent.posY = ent.targetY;
                ent.isMoving = false;
            }
            else
            {
                float ratio = step / dist;
                ent.posX += dx * ratio;
                ent.posY += dy * ratio;
                ent.isMoving = true;
            }
        }
        else
        {
            ent.isMoving = false;
        }
    }
}
