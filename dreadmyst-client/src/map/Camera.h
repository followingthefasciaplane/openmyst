#pragma once

#include <cstdint>

// Camera - smooth follow camera for isometric map.
//
// Decompiled from World::updateCamera at 0x005354c0.
// Camera follows the player entity using isometric projection.
//
// Isometric projection (entity grid position -> screen target):
//   targetX = screenCenterX - (entityX * 32 - entityY * 32)
//   targetY = screenCenterY - (entityY * 16 + entityX * 16)
//
// Follow behavior:
//   - Teleport if distance > screenHeight (e.g., map change)
//   - Smooth follow if distance > 15: speed = 2 * dt * distance
//   - No move if distance <= 15 (dead zone)
//
// Zoom level (binary offset 0xB4 in GameMap):
//   - Normal: 2
//   - Mounted: 10 (2 + 8)

class Camera
{
public:
    // Set the screen center (typically half window dimensions)
    void setScreenCenter(float cx, float cy) { m_screenCenterX = cx; m_screenCenterY = cy; }

    // Set the screen height (used for teleport threshold)
    void setScreenHeight(float h) { m_screenHeight = h; }

    // Update camera to follow a world-space entity position.
    // entityX/entityY are grid coordinates (not pixel).
    // deltaTime in seconds.
    void update(float entityX, float entityY, float deltaTime);

    // Teleport camera to target position immediately
    void teleportTo(float entityX, float entityY);

    // Current camera position (pixel offset for rendering)
    float getX() const { return m_x; }
    float getY() const { return m_y; }

    // Camera offset (e.g., screen shake)
    void setOffset(float ox, float oy) { m_offsetX = ox; m_offsetY = oy; }

private:
    // Compute the camera target from entity grid position
    void computeTarget(float entityX, float entityY, float& outX, float& outY) const;

    float m_x = 0.0f;
    float m_y = 0.0f;

    float m_screenCenterX = 640.0f;
    float m_screenCenterY = 360.0f;
    float m_screenHeight  = 720.0f;

    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
};
