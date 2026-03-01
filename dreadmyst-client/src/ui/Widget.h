#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <cstdint>

// Widget - base class for all UI elements.
// Matches binary's RenderObject/MouseableNode hierarchy pattern.
// Input dispatch: top-down through children in reverse order (front-to-back),
// first widget returning true consumes the event.

class Widget
{
public:
    virtual ~Widget() = default;

    virtual void update(float dt);
    virtual void render(sf::RenderTarget& target);

    // Input handlers - return true if event was consumed
    virtual bool handleMouseMove(float mx, float my);
    virtual bool handleMousePress(float mx, float my, sf::Mouse::Button btn);
    virtual bool handleMouseRelease(float mx, float my, sf::Mouse::Button btn);
    virtual bool handleKeyPress(sf::Keyboard::Key key);
    virtual bool handleTextInput(uint32_t unicode);

    // Hierarchy
    Widget* addChild(std::unique_ptr<Widget> child);
    void removeChild(Widget* child);
    const std::vector<std::unique_ptr<Widget>>& getChildren() const { return m_children; }

    // Screen-space position (walks parent chain)
    float getScreenX() const;
    float getScreenY() const;
    bool containsPoint(float px, float py) const;

    // Accessors
    void setPosition(float x, float y) { m_x = x; m_y = y; }
    void setSize(float w, float h) { m_width = w; m_height = h; }
    void setVisible(bool v) { m_visible = v; }
    void setEnabled(bool e) { m_enabled = e; }

    bool isVisible() const { return m_visible; }
    bool isEnabled() const { return m_enabled; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }
    Widget* getParent() const { return m_parent; }

protected:
    float m_x = 0, m_y = 0;
    float m_width = 0, m_height = 0;
    bool m_visible = true;
    bool m_enabled = true;
    Widget* m_parent = nullptr;
    std::vector<std::unique_ptr<Widget>> m_children;
};
