#include "Widget.h"
#include <algorithm>

void Widget::update(float dt)
{
    for (auto& child : m_children)
    {
        if (child->isVisible())
            child->update(dt);
    }
}

void Widget::render(sf::RenderTarget& target)
{
    if (!m_visible)
        return;

    for (auto& child : m_children)
    {
        if (child->isVisible())
            child->render(target);
    }
}

// Input dispatch: iterate children in reverse (front-to-back) so topmost widget gets input first.
// Matches binary's mayInput() pattern.

bool Widget::handleMouseMove(float mx, float my)
{
    for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; --i)
    {
        auto& child = m_children[i];
        if (child->isVisible() && child->isEnabled() && child->handleMouseMove(mx, my))
            return true;
    }
    return false;
}

bool Widget::handleMousePress(float mx, float my, sf::Mouse::Button btn)
{
    for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; --i)
    {
        auto& child = m_children[i];
        if (child->isVisible() && child->isEnabled() && child->handleMousePress(mx, my, btn))
            return true;
    }
    return false;
}

bool Widget::handleMouseRelease(float mx, float my, sf::Mouse::Button btn)
{
    for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; --i)
    {
        auto& child = m_children[i];
        if (child->isVisible() && child->isEnabled() && child->handleMouseRelease(mx, my, btn))
            return true;
    }
    return false;
}

bool Widget::handleKeyPress(sf::Keyboard::Key key)
{
    for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; --i)
    {
        auto& child = m_children[i];
        if (child->isVisible() && child->isEnabled() && child->handleKeyPress(key))
            return true;
    }
    return false;
}

bool Widget::handleTextInput(uint32_t unicode)
{
    for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; --i)
    {
        auto& child = m_children[i];
        if (child->isVisible() && child->isEnabled() && child->handleTextInput(unicode))
            return true;
    }
    return false;
}

Widget* Widget::addChild(std::unique_ptr<Widget> child)
{
    child->m_parent = this;
    Widget* ptr = child.get();
    m_children.push_back(std::move(child));
    return ptr;
}

void Widget::removeChild(Widget* child)
{
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::unique_ptr<Widget>& c) { return c.get() == child; });
    if (it != m_children.end())
    {
        (*it)->m_parent = nullptr;
        m_children.erase(it);
    }
}

float Widget::getScreenX() const
{
    float sx = m_x;
    if (m_parent)
        sx += m_parent->getScreenX();
    return sx;
}

float Widget::getScreenY() const
{
    float sy = m_y;
    if (m_parent)
        sy += m_parent->getScreenY();
    return sy;
}

bool Widget::containsPoint(float px, float py) const
{
    float sx = getScreenX();
    float sy = getScreenY();
    return px >= sx && px < sx + m_width && py >= sy && py < sy + m_height;
}
