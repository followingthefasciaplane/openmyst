#pragma once

#include "Widget.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

// UIManager - singleton that owns all top-level UI widgets and dispatches input.
// All UI is drawn after world content, parent-to-child traversal.

class UIManager
{
public:
    static UIManager& instance();

    void addRoot(std::unique_ptr<Widget> widget);
    void removeRoot(Widget* widget);

    // Called from Application::input() - returns true if UI consumed the event
    bool handleEvent(const sf::Event& event, float mouseX, float mouseY);

    // Called each frame
    void update(float dt);

    // Called from Application::render() after entity rendering
    void render(sf::RenderTarget& target);

    // Did UI consume mouse input this frame?
    bool wasInputConsumed() const { return m_inputConsumed; }

    // Reset input consumed flag (call at start of frame)
    void resetInputConsumed() { m_inputConsumed = false; }

    // Check if mouse is over any UI widget
    bool isMouseOverUI(float mx, float my) const;

private:
    UIManager() = default;

    std::vector<std::unique_ptr<Widget>> m_roots;
    bool m_inputConsumed = false;
};

#define sUI UIManager::instance()
