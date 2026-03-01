#include "ChatWindow.h"
#include "ui/FontManager.h"
#include "core/Log.h"
#include <algorithm>

void ChatWindow::init(unsigned int screenHeight)
{
    // Position bottom-left, above toolbar area
    setPosition(10.f, static_cast<float>(screenHeight) - m_chatHeight - 85.f);
    setSize(m_chatWidth, m_chatHeight);

    // 9-slice console panel
    auto panel = std::make_unique<Panel>();
    panel->loadFromPrefix("console");
    panel->setPosition(0.f, 0.f);
    panel->setSize(m_chatWidth, m_chatHeight);
    panel->setDraggable(true);
    m_panel = static_cast<Panel*>(addChild(std::move(panel)));

    // Scroll buttons (positioned at right edge of panel)
    auto scrollUp = std::make_unique<Button>();
    scrollUp->load("console_scroll_up");
    scrollUp->setPosition(m_chatWidth - 26.f, 8.f);
    scrollUp->setOnClick([this]() { this->scrollUp(); });
    m_scrollUp = static_cast<Button*>(addChild(std::move(scrollUp)));

    auto scrollDown = std::make_unique<Button>();
    scrollDown->load("console_scroll_down");
    scrollDown->setPosition(m_chatWidth - 26.f, m_chatHeight - 26.f);
    scrollDown->setOnClick([this]() { this->scrollDown(); });
    m_scrollDown = static_cast<Button*>(addChild(std::move(scrollDown)));

    LOG_INFO("ChatWindow initialized");
}

void ChatWindow::addMessage(const std::string& msg)
{
    m_messages.push_back(msg);

    // Auto-scroll to bottom when new message arrives
    int totalLines = static_cast<int>(m_messages.size());
    if (totalLines > m_maxVisibleLines)
        m_scrollOffset = totalLines - m_maxVisibleLines;

    rebuildLabels();
}

void ChatWindow::scrollUp()
{
    if (m_scrollOffset > 0)
    {
        m_scrollOffset--;
        rebuildLabels();
    }
}

void ChatWindow::scrollDown()
{
    int totalLines = static_cast<int>(m_messages.size());
    if (m_scrollOffset < totalLines - m_maxVisibleLines)
    {
        m_scrollOffset++;
        rebuildLabels();
    }
}

void ChatWindow::rebuildLabels()
{
    const sf::Font* font = sFontMgr.getDefault();
    if (!font)
        font = sFontMgr.getFont("arial.ttf");
    if (!font)
        return;

    // Remove old message labels from panel
    for (auto* lbl : m_messageLabels)
    {
        if (m_panel)
            m_panel->removeChild(lbl);
    }
    m_messageLabels.clear();

    // Add visible messages
    int startIdx = std::max(0, m_scrollOffset);
    int endIdx = std::min(static_cast<int>(m_messages.size()), startIdx + m_maxVisibleLines);

    float lineHeight = 20.f;
    float textStartX = 12.f;
    float textStartY = 12.f;

    for (int i = startIdx; i < endIdx; ++i)
    {
        auto lbl = std::make_unique<Label>();
        lbl->init(*font, m_messages[i], 12);
        lbl->setColor(sf::Color(220, 220, 220));
        lbl->setPosition(textStartX, textStartY + static_cast<float>(i - startIdx) * lineHeight);
        Label* ptr = static_cast<Label*>(m_panel->addChild(std::move(lbl)));
        m_messageLabels.push_back(ptr);
    }
}

void ChatWindow::render(sf::RenderTarget& target)
{
    if (!m_visible)
        return;

    Widget::render(target);
}
