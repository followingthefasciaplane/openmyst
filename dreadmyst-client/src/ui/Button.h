#pragma once

#include "Widget.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>

// Button - 3-state button (idle/hover/pressed).
// Matches binary's Button class.

class Button : public Widget
{
public:
    // Load 3 textures: <baseName>_idle.png, <baseName>_hover.png, <baseName>_press.png
    bool load(const std::string& baseName);

    void setOnClick(std::function<void()> cb) { m_onClick = std::move(cb); }

    void render(sf::RenderTarget& target) override;
    bool handleMouseMove(float mx, float my) override;
    bool handleMousePress(float mx, float my, sf::Mouse::Button btn) override;
    bool handleMouseRelease(float mx, float my, sf::Mouse::Button btn) override;

private:
    enum class State { Idle, Hover, Pressed };
    State m_state = State::Idle;

    std::shared_ptr<sf::Texture> m_texIdle, m_texHover, m_texPress;
    std::function<void()> m_onClick;
};
