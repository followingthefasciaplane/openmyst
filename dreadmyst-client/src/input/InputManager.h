#pragma once

#include <SFML/Window/Keyboard.hpp>

// InputManager - reads WASD/arrow key state and computes movement direction.
//
// Binary: GP_Client_RequestMove at 0x0068157c (RTTI string).
// Movement is isometric: W=North, S=South, A=West, D=East.
// Orientation is 8-directional matching UnitDefines::Direction.

class InputManager
{
public:
    // Poll keyboard state and compute movement vector.
    // Returns true if the player is requesting movement this frame.
    bool update(float dt);

    // Movement delta in grid coordinates (to apply to player position)
    float getDeltaX() const { return m_deltaX; }
    float getDeltaY() const { return m_deltaY; }

    // 8-directional orientation (0=S, 1=SW, 2=W, 3=NW, 4=N, 5=NE, 6=E, 7=SE)
    float getOrientation() const { return m_orientation; }

    // Whether movement state changed since last frame
    bool movementChanged() const { return m_movementChanged; }

    // Set movement speed (cells per second)
    void setMoveSpeed(float speed) { m_moveSpeed = speed; }

private:
    float m_deltaX = 0.f;
    float m_deltaY = 0.f;
    float m_orientation = 0.f;
    float m_moveSpeed = 4.0f;  // cells per second
    bool  m_wasMoving = false;
    bool  m_movementChanged = false;
};
