#include "Button.h"
#include "resource/ResourceManager.h"
#include "core/Log.h"

bool Button::load(const std::string& baseName)
{
    m_texIdle  = sResourceMgr.getTexture(baseName + "_idle.png");
    m_texHover = sResourceMgr.getTexture(baseName + "_hover.png");
    m_texPress = sResourceMgr.getTexture(baseName + "_press.png");

    if (!m_texIdle)
    {
        LOG_WARN("Button: failed to load idle texture for '%s'", baseName.c_str());
        return false;
    }

    // Auto-size from idle texture
    auto sz = m_texIdle->getSize();
    m_width  = static_cast<float>(sz.x);
    m_height = static_cast<float>(sz.y);
    return true;
}

void Button::render(sf::RenderTarget& target)
{
    if (!m_visible)
        return;

    const sf::Texture* tex = nullptr;
    switch (m_state)
    {
        case State::Pressed: tex = m_texPress ? m_texPress.get() : (m_texIdle ? m_texIdle.get() : nullptr); break;
        case State::Hover:   tex = m_texHover ? m_texHover.get() : (m_texIdle ? m_texIdle.get() : nullptr); break;
        default:             tex = m_texIdle  ? m_texIdle.get()  : nullptr; break;
    }

    if (tex)
    {
        sf::Sprite spr(*tex);
        spr.setPosition({getScreenX(), getScreenY()});
        target.draw(spr);
    }

    Widget::render(target);
}

bool Button::handleMouseMove(float mx, float my)
{
    if (containsPoint(mx, my))
    {
        if (m_state != State::Pressed)
            m_state = State::Hover;
        return true;
    }
    else
    {
        m_state = State::Idle;
    }
    return false;
}

bool Button::handleMousePress(float mx, float my, sf::Mouse::Button btn)
{
    if (btn == sf::Mouse::Button::Left && containsPoint(mx, my))
    {
        m_state = State::Pressed;
        return true;
    }
    return false;
}

bool Button::handleMouseRelease(float mx, float my, sf::Mouse::Button btn)
{
    if (btn == sf::Mouse::Button::Left && m_state == State::Pressed)
    {
        if (containsPoint(mx, my))
        {
            m_state = State::Hover;
            if (m_onClick)
                m_onClick();
        }
        else
        {
            m_state = State::Idle;
        }
        return true;
    }
    return false;
}
