#pragma once

#include "Widget.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

// Panel - 9-slice resizable panel.
// Matches binary's ExpandableWindow at 0x004f3910.
// Loads 9 texture pieces: TopLeft, TopAcross, TopRight, LeftUp, Center, RightUp,
// BottomLeft, BottomAcross, BottomRight. Corners keep size, edges stretch, center fills.

class Panel : public Widget
{
public:
    // Load 9 textures by prefix (e.g. "console" loads console_top_left.png etc.)
    bool loadFromPrefix(const std::string& prefix);

    void setDraggable(bool d) { m_draggable = d; }

    void render(sf::RenderTarget& target) override;
    bool handleMousePress(float mx, float my, sf::Mouse::Button btn) override;
    bool handleMouseMove(float mx, float my) override;
    bool handleMouseRelease(float mx, float my, sf::Mouse::Button btn) override;

private:
    std::shared_ptr<sf::Texture> m_texTL, m_texTA, m_texTR;
    std::shared_ptr<sf::Texture> m_texLU, m_texC,  m_texRU;
    std::shared_ptr<sf::Texture> m_texBL, m_texBA, m_texBR;

    bool m_draggable = false;
    bool m_dragging = false;
    float m_dragOffX = 0, m_dragOffY = 0;
};
