#include "UIManager.h"
#include <algorithm>

UIManager& UIManager::instance()
{
    static UIManager inst;
    return inst;
}

void UIManager::addRoot(std::unique_ptr<Widget> widget)
{
    m_roots.push_back(std::move(widget));
}

void UIManager::removeRoot(Widget* widget)
{
    auto it = std::find_if(m_roots.begin(), m_roots.end(),
        [widget](const std::unique_ptr<Widget>& w) { return w.get() == widget; });
    if (it != m_roots.end())
        m_roots.erase(it);
}

bool UIManager::handleEvent(const sf::Event& event, float mouseX, float mouseY)
{
    // Mouse move - dispatch to all roots (front-to-back = reverse order)
    if (event.is<sf::Event::MouseMoved>())
    {
        for (int i = static_cast<int>(m_roots.size()) - 1; i >= 0; --i)
        {
            auto& w = m_roots[i];
            if (w->isVisible() && w->handleMouseMove(mouseX, mouseY))
            {
                m_inputConsumed = true;
                return true;
            }
        }
        return false;
    }

    // Mouse press
    if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>())
    {
        for (int i = static_cast<int>(m_roots.size()) - 1; i >= 0; --i)
        {
            auto& w = m_roots[i];
            if (w->isVisible() && w->handleMousePress(mouseX, mouseY, mbp->button))
            {
                m_inputConsumed = true;
                return true;
            }
        }
        return false;
    }

    // Mouse release
    if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>())
    {
        for (int i = static_cast<int>(m_roots.size()) - 1; i >= 0; --i)
        {
            auto& w = m_roots[i];
            if (w->isVisible() && w->handleMouseRelease(mouseX, mouseY, mbr->button))
            {
                m_inputConsumed = true;
                return true;
            }
        }
        return false;
    }

    // Key press
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>())
    {
        for (int i = static_cast<int>(m_roots.size()) - 1; i >= 0; --i)
        {
            auto& w = m_roots[i];
            if (w->isVisible() && w->handleKeyPress(kp->code))
            {
                m_inputConsumed = true;
                return true;
            }
        }
        return false;
    }

    // Text input
    if (const auto* ti = event.getIf<sf::Event::TextEntered>())
    {
        for (int i = static_cast<int>(m_roots.size()) - 1; i >= 0; --i)
        {
            auto& w = m_roots[i];
            if (w->isVisible() && w->handleTextInput(ti->unicode))
            {
                m_inputConsumed = true;
                return true;
            }
        }
        return false;
    }

    return false;
}

void UIManager::update(float dt)
{
    for (auto& w : m_roots)
    {
        if (w->isVisible())
            w->update(dt);
    }
}

void UIManager::render(sf::RenderTarget& target)
{
    for (auto& w : m_roots)
    {
        if (w->isVisible())
            w->render(target);
    }
}

bool UIManager::isMouseOverUI(float mx, float my) const
{
    for (int i = static_cast<int>(m_roots.size()) - 1; i >= 0; --i)
    {
        if (m_roots[i]->isVisible() && m_roots[i]->containsPoint(mx, my))
            return true;
    }
    return false;
}
