#pragma once

#include "ui/Widget.h"
#include "ui/Panel.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include <vector>
#include <string>

// ChatWindow - displays received chat messages in a 9-slice console panel.
// Uses console_* textures for the panel, scroll buttons for navigation.
// Position: bottom-left corner (matching binary layout).

class ChatWindow : public Widget
{
public:
    void init(unsigned int screenHeight);

    // Add a message to the chat history
    void addMessage(const std::string& msg);

    void render(sf::RenderTarget& target) override;

private:
    void rebuildLabels();
    void scrollUp();
    void scrollDown();

    Panel* m_panel = nullptr;
    Button* m_scrollUp = nullptr;
    Button* m_scrollDown = nullptr;

    std::vector<std::string> m_messages;
    std::vector<Label*> m_messageLabels;

    int m_scrollOffset = 0;
    int m_maxVisibleLines = 8;
    float m_chatWidth = 400.f;
    float m_chatHeight = 250.f;
};
