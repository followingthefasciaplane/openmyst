#include "Camera.h"

#include <cmath>
#include <algorithm>

// Camera implementation.
//
// Decompiled from World::updateCamera at 0x005354c0 and
// World::getControllerCameraPosition at 0x00538140.
//
// Constants from binary:
//   TILE_HALF_W = 32.0f
//   TILE_HALF_H = 16.0f
//   Dead zone = 15.0f
//   Speed = 2.0f * dt * distance

static constexpr float TILE_HALF_W = 32.0f;
static constexpr float TILE_HALF_H = 16.0f;
static constexpr float DEAD_ZONE   = 15.0f;

void Camera::computeTarget(float entityX, float entityY, float& outX, float& outY) const
{
    // Binary: FUN_00538140 (World::getControllerCameraPosition)
    // targetX = screenCenter.x - (entityX * 32 - entityY * 32)
    // targetY = screenCenter.y - (entityY * 16 + entityX * 16)
    outX = m_screenCenterX - (entityX * TILE_HALF_W - entityY * TILE_HALF_W) + m_offsetX;
    outY = m_screenCenterY - (entityY * TILE_HALF_H + entityX * TILE_HALF_H) + m_offsetY;
}

void Camera::update(float entityX, float entityY, float deltaTime)
{
    float targetX, targetY;
    computeTarget(entityX, entityY, targetX, targetY);

    float dx = targetX - m_x;
    float dy = targetY - m_y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist >= m_screenHeight)
    {
        // Teleport camera if too far (map change, etc.)
        m_x = targetX;
        m_y = targetY;
    }
    else if (dist > DEAD_ZONE)
    {
        // Smooth follow: speed proportional to distance
        // Binary: speed = dt * dist + dt * dist = 2 * dt * dist
        float speed = 2.0f * deltaTime * dist;
        speed = std::min(speed, dist);

        float newDx = targetX - m_x;
        float newDy = targetY - m_y;
        float newDist = std::sqrt(newDx * newDx + newDy * newDy);

        if (speed > 0.0f && newDist > 0.0f)
        {
            if (speed < newDist)
            {
                float ratio = speed / newDist;
                m_x += newDx * ratio;
                m_y += newDy * ratio;
            }
            else
            {
                m_x = targetX;
                m_y = targetY;
            }
        }
    }
}

void Camera::teleportTo(float entityX, float entityY)
{
    computeTarget(entityX, entityY, m_x, m_y);
}
