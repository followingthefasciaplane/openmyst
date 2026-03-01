#pragma once

#include "SpriteSheet.h"
#include <SFML/Graphics.hpp>

// SpriteAnimator - per-entity animation state machine.
//
// Tracks current animation (AnimId), direction, frame index, and timer.
// Call update(dt) each frame; getCurrentFrame() returns the IntRect to draw.

class SpriteAnimator
{
public:
    void setAnimation(int animId, float totalDuration, int totalFrames, bool loop = true);
    void setDirection(int dir);
    void update(float dt);

    sf::IntRect getCurrentFrame(const SpriteSheet& sheet) const;

    int  getAnimId()    const { return m_currentAnim; }
    int  getDirection() const { return m_direction; }
    bool isFinished()   const { return m_finished; }

private:
    int   m_currentAnim   = 0;
    int   m_direction      = 0;    // UnitDefines::Direction (0-7)
    int   m_currentFrame   = 0;
    int   m_totalFrames    = 1;
    float m_frameTimer     = 0.f;
    float m_frameDuration  = 0.1f; // seconds per frame
    bool  m_looping        = true;
    bool  m_finished       = false;
};
