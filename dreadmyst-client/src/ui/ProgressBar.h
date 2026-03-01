#pragma once

#include "Widget.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

// ProgressBar - fill bar for HP, MP, XP, cast bars.
// Uses a background texture and a fill texture that's clipped based on value.

class ProgressBar : public Widget
{
public:
    // Load background and fill textures by name
    bool load(const std::string& bgName, const std::string& fillName);

    // Set fill value (clamped 0..1)
    void setValue(float v);
    float getValue() const { return m_value; }

    void setFillColor(sf::Color c) { m_fillColor = c; }

    void render(sf::RenderTarget& target) override;

private:
    std::shared_ptr<sf::Texture> m_bgTex;
    std::shared_ptr<sf::Texture> m_fillTex;
    float m_value = 1.0f;
    sf::Color m_fillColor = sf::Color::White;
};
