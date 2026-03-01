#include "SpriteAnimator.h"

void SpriteAnimator::setAnimation(int animId, float totalDuration, int totalFrames, bool loop)
{
    if (animId == m_currentAnim && totalFrames == m_totalFrames)
        return;  // already playing this animation

    m_currentAnim  = animId;
    m_totalFrames  = (totalFrames > 0) ? totalFrames : 1;
    m_frameDuration = (totalDuration > 0.f && m_totalFrames > 0)
                      ? totalDuration / static_cast<float>(m_totalFrames)
                      : 0.1f;
    m_looping      = loop;
    m_currentFrame = 0;
    m_frameTimer   = 0.f;
    m_finished     = false;
}

void SpriteAnimator::setDirection(int dir)
{
    if (dir >= 0 && dir < 8)
        m_direction = dir;
}

void SpriteAnimator::update(float dt)
{
    if (m_finished || m_totalFrames <= 1)
        return;

    m_frameTimer += dt;

    while (m_frameTimer >= m_frameDuration)
    {
        m_frameTimer -= m_frameDuration;
        ++m_currentFrame;

        if (m_currentFrame >= m_totalFrames)
        {
            if (m_looping)
                m_currentFrame = 0;
            else
            {
                m_currentFrame = m_totalFrames - 1;
                m_finished = true;
                return;
            }
        }
    }
}

sf::IntRect SpriteAnimator::getCurrentFrame(const SpriteSheet& sheet) const
{
    return sheet.getFrameRect(m_direction, m_currentFrame);
}
