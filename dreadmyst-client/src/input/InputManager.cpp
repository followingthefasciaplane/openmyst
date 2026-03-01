#include "InputManager.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

// Isometric movement directions in grid coordinates:
//   North (W key): row--, col-- → grid delta (-1, -1)
//   South (S key): row++, col++ → grid delta (+1, +1)
//   West  (A key): row++, col-- → grid delta (-1, +1)
//   East  (D key): row--, col++ → grid delta (+1, -1)
//
// Orientation values match UnitDefines::Direction:
//   0=South, 1=SouthWest, 2=West, 3=NorthWest, 4=North, 5=NorthEast, 6=East, 7=SouthEast

bool InputManager::update(float dt)
{
    bool up    = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
    bool down  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
    bool left  = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);

    // Compute grid-space movement direction
    // In isometric: up=North(-1,-1), down=South(+1,+1), left=West(-1,+1), right=East(+1,-1)
    float dirX = 0.f, dirY = 0.f;

    if (up)    { dirX -= 1.f; dirY -= 1.f; }  // North
    if (down)  { dirX += 1.f; dirY += 1.f; }  // South
    if (left)  { dirX -= 1.f; dirY += 1.f; }  // West
    if (right) { dirX += 1.f; dirY -= 1.f; }  // East

    bool isMoving = (dirX != 0.f || dirY != 0.f);

    m_movementChanged = (isMoving != m_wasMoving);
    m_wasMoving = isMoving;

    if (!isMoving)
    {
        m_deltaX = 0.f;
        m_deltaY = 0.f;
        return false;
    }

    // Normalize diagonal movement
    float len = std::sqrt(dirX * dirX + dirY * dirY);
    if (len > 0.f)
    {
        dirX /= len;
        dirY /= len;
    }

    m_deltaX = dirX * m_moveSpeed * dt;
    m_deltaY = dirY * m_moveSpeed * dt;

    // Compute 8-directional orientation
    // dirX is column direction, dirY is row direction
    if (dirX > 0.3f && dirY > 0.3f)        m_orientation = 0.f;  // South (+col, +row)
    else if (dirX < -0.3f && dirY > 0.3f)   m_orientation = 2.f;  // West (-col, +row)
    else if (dirX < -0.3f && dirY < -0.3f)  m_orientation = 4.f;  // North (-col, -row)
    else if (dirX > 0.3f && dirY < -0.3f)   m_orientation = 6.f;  // East (+col, -row)
    else if (dirX > 0.3f)                    m_orientation = 7.f;  // SouthEast
    else if (dirX < -0.3f)                   m_orientation = 3.f;  // NorthWest
    else if (dirY > 0.3f)                    m_orientation = 1.f;  // SouthWest
    else if (dirY < -0.3f)                   m_orientation = 5.f;  // NorthEast

    m_movementChanged = true;  // Always re-send while moving
    return true;
}
