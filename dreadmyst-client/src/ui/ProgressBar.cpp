#include "ProgressBar.h"
#include "resource/ResourceManager.h"
#include "core/Log.h"
#include <algorithm>

bool ProgressBar::load(const std::string& bgName, const std::string& fillName)
{
    m_bgTex   = sResourceMgr.getTexture(bgName);
    m_fillTex = sResourceMgr.getTexture(fillName);

    if (m_bgTex)
    {
        auto sz = m_bgTex->getSize();
        m_width  = static_cast<float>(sz.x);
        m_height = static_cast<float>(sz.y);
    }

    return m_bgTex != nullptr;
}

void ProgressBar::setValue(float v)
{
    m_value = std::clamp(v, 0.f, 1.f);
}

void ProgressBar::render(sf::RenderTarget& target)
{
    if (!m_visible)
        return;

    float sx = getScreenX();
    float sy = getScreenY();

    // Draw background
    if (m_bgTex)
    {
        sf::Sprite bg(*m_bgTex);
        bg.setPosition({sx, sy});
        // Scale to widget size
        auto bgsz = m_bgTex->getSize();
        if (bgsz.x > 0 && bgsz.y > 0)
            bg.setScale({m_width / static_cast<float>(bgsz.x), m_height / static_cast<float>(bgsz.y)});
        target.draw(bg);
    }

    // Draw fill (clipped by value)
    if (m_fillTex && m_value > 0.f)
    {
        auto fsz = m_fillTex->getSize();
        int clipW = static_cast<int>(static_cast<float>(fsz.x) * m_value);
        if (clipW > 0)
        {
            sf::Sprite fill(*m_fillTex);
            fill.setTextureRect(sf::IntRect({0, 0}, {clipW, static_cast<int>(fsz.y)}));
            fill.setPosition({sx, sy});
            fill.setColor(m_fillColor);
            // Scale to match widget dimensions
            if (fsz.x > 0 && fsz.y > 0)
                fill.setScale({m_width / static_cast<float>(fsz.x), m_height / static_cast<float>(fsz.y)});
            target.draw(fill);
        }
    }

    Widget::render(target);
}
