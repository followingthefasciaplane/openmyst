#include "Panel.h"
#include "resource/ResourceManager.h"
#include "core/Log.h"

bool Panel::loadFromPrefix(const std::string& prefix)
{
    m_texTL = sResourceMgr.getTexture(prefix + "_top_left.png");
    m_texTA = sResourceMgr.getTexture(prefix + "_top_across.png");
    m_texTR = sResourceMgr.getTexture(prefix + "_top_right.png");
    m_texLU = sResourceMgr.getTexture(prefix + "_left_up.png");
    m_texC  = sResourceMgr.getTexture(prefix + "_center.png");
    m_texRU = sResourceMgr.getTexture(prefix + "_right_up.png");
    m_texBL = sResourceMgr.getTexture(prefix + "_bottom_left.png");
    m_texBA = sResourceMgr.getTexture(prefix + "_bottom_across.png");
    m_texBR = sResourceMgr.getTexture(prefix + "_bottom_right.png");

    if (!m_texTL || !m_texBR)
    {
        LOG_WARN("Panel: failed to load 9-slice textures for prefix '%s'", prefix.c_str());
        return false;
    }

    return true;
}

void Panel::render(sf::RenderTarget& target)
{
    if (!m_visible)
        return;

    float sx = getScreenX();
    float sy = getScreenY();

    // Corner dimensions
    float tlW = m_texTL ? static_cast<float>(m_texTL->getSize().x) : 0;
    float tlH = m_texTL ? static_cast<float>(m_texTL->getSize().y) : 0;
    float trW = m_texTR ? static_cast<float>(m_texTR->getSize().x) : 0;
    float trH = m_texTR ? static_cast<float>(m_texTR->getSize().y) : 0;
    float blW = m_texBL ? static_cast<float>(m_texBL->getSize().x) : 0;
    float blH = m_texBL ? static_cast<float>(m_texBL->getSize().y) : 0;
    float brW = m_texBR ? static_cast<float>(m_texBR->getSize().x) : 0;
    float brH = m_texBR ? static_cast<float>(m_texBR->getSize().y) : 0;

    float innerW = m_width - tlW - trW;
    float innerH = m_height - tlH - blH;

    auto drawScaled = [&](const std::shared_ptr<sf::Texture>& tex, float x, float y, float w, float h)
    {
        if (!tex) return;
        sf::Sprite spr(*tex);
        spr.setPosition({x, y});
        auto tsz = tex->getSize();
        if (tsz.x > 0 && tsz.y > 0)
            spr.setScale({w / static_cast<float>(tsz.x), h / static_cast<float>(tsz.y)});
        target.draw(spr);
    };

    // Center fill
    if (innerW > 0 && innerH > 0)
        drawScaled(m_texC, sx + tlW, sy + tlH, innerW, innerH);

    // Edges
    if (innerW > 0)
    {
        float taH = m_texTA ? static_cast<float>(m_texTA->getSize().y) : tlH;
        float baH = m_texBA ? static_cast<float>(m_texBA->getSize().y) : blH;
        drawScaled(m_texTA, sx + tlW, sy, innerW, taH);
        drawScaled(m_texBA, sx + blW, sy + m_height - baH, innerW, baH);
    }
    if (innerH > 0)
    {
        float luW = m_texLU ? static_cast<float>(m_texLU->getSize().x) : tlW;
        float ruW = m_texRU ? static_cast<float>(m_texRU->getSize().x) : trW;
        drawScaled(m_texLU, sx, sy + tlH, luW, innerH);
        drawScaled(m_texRU, sx + m_width - ruW, sy + trH, ruW, innerH);
    }

    // Corners (drawn at natural size)
    if (m_texTL) { sf::Sprite s(*m_texTL); s.setPosition({sx, sy}); target.draw(s); }
    if (m_texTR) { sf::Sprite s(*m_texTR); s.setPosition({sx + m_width - trW, sy}); target.draw(s); }
    if (m_texBL) { sf::Sprite s(*m_texBL); s.setPosition({sx, sy + m_height - blH}); target.draw(s); }
    if (m_texBR) { sf::Sprite s(*m_texBR); s.setPosition({sx + m_width - brW, sy + m_height - brH}); target.draw(s); }

    // Render children on top
    Widget::render(target);
}

bool Panel::handleMousePress(float mx, float my, sf::Mouse::Button btn)
{
    // Check children first
    if (Widget::handleMousePress(mx, my, btn))
        return true;

    if (containsPoint(mx, my))
    {
        if (m_draggable && btn == sf::Mouse::Button::Left)
        {
            m_dragging = true;
            m_dragOffX = mx - getScreenX();
            m_dragOffY = my - getScreenY();
        }
        return true; // Consume click on panel body
    }
    return false;
}

bool Panel::handleMouseMove(float mx, float my)
{
    if (m_dragging)
    {
        m_x = mx - m_dragOffX;
        m_y = my - m_dragOffY;
        // If parented, adjust for parent offset
        if (m_parent)
        {
            m_x -= m_parent->getScreenX();
            m_y -= m_parent->getScreenY();
        }
        return true;
    }

    return Widget::handleMouseMove(mx, my);
}

bool Panel::handleMouseRelease(float mx, float my, sf::Mouse::Button btn)
{
    if (m_dragging && btn == sf::Mouse::Button::Left)
    {
        m_dragging = false;
        return true;
    }
    return Widget::handleMouseRelease(mx, my, btn);
}
