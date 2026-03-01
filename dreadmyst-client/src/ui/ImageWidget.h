#pragma once

#include "Widget.h"
#include "resource/ResourceManager.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

// ImageWidget - simple static texture display.

class ImageWidget : public Widget
{
public:
    bool load(const std::string& textureName)
    {
        m_texture = sResourceMgr.getTexture(textureName);
        if (m_texture)
        {
            auto sz = m_texture->getSize();
            m_width  = static_cast<float>(sz.x);
            m_height = static_cast<float>(sz.y);
            return true;
        }
        return false;
    }

    void render(sf::RenderTarget& target) override
    {
        if (!m_visible)
            return;

        if (m_texture)
        {
            sf::Sprite spr(*m_texture);
            spr.setPosition({getScreenX(), getScreenY()});
            target.draw(spr);
        }

        Widget::render(target);
    }

private:
    std::shared_ptr<sf::Texture> m_texture;
};
