#pragma once

#include "Widget.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

// Label - simple text display widget.
// Uses std::optional<sf::Text> because SFML 3's sf::Text has no default constructor.

class Label : public Widget
{
public:
    Label() = default;

    Label(const sf::Font& font, const std::string& text = "", unsigned int size = 14)
        : m_text(sf::Text(font, text, size))
    {
        updateBounds();
    }

    void init(const sf::Font& font, const std::string& text = "", unsigned int size = 14)
    {
        m_text.emplace(font, text, size);
        updateBounds();
    }

    void setText(const std::string& text)
    {
        if (m_text)
        {
            m_text->setString(text);
            updateBounds();
        }
    }

    void setColor(sf::Color color)
    {
        if (m_text)
            m_text->setFillColor(color);
    }

    void setCharacterSize(unsigned int size)
    {
        if (m_text)
        {
            m_text->setCharacterSize(size);
            updateBounds();
        }
    }

    void render(sf::RenderTarget& target) override
    {
        if (!m_visible || !m_text)
            return;

        m_text->setPosition({getScreenX(), getScreenY()});
        target.draw(*m_text);

        Widget::render(target);
    }

private:
    void updateBounds()
    {
        if (!m_text) return;
        auto bounds = m_text->getLocalBounds();
        m_width  = bounds.size.x;
        m_height = bounds.size.y;
    }

    std::optional<sf::Text> m_text;
};
