#include "game/MutualObject.h"
#include <cmath>

float MutualObject::distanceTo(const MutualObject& other) const {
    return distanceTo(other.m_posX, other.m_posY);
}

float MutualObject::distanceTo(float x, float y) const {
    float dx = m_posX - x;
    float dy = m_posY - y;
    return std::sqrt(dx * dx + dy * dy);
}
