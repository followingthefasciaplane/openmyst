#pragma once

#include "ui/Widget.h"
#include "ui/ImageWidget.h"
#include "ui/Button.h"
#include "ui/ProgressBar.h"

// Toolbar - bottom-center action bar with menu buttons.
// Textures: toolbar_base.png (861x69), 6 menu buttons (36x36 each), xp_bar.png (633x12).
// Button layout from binary: character, inventory, spells, quests, social, options.

class Toolbar : public Widget
{
public:
    void init(unsigned int screenWidth, unsigned int screenHeight);
    void updateXP(uint32_t xp, uint32_t xpToLevel);
    void reposition(unsigned int screenWidth, unsigned int screenHeight);

private:
    ImageWidget* m_base = nullptr;
    Button* m_btnCharacter = nullptr;
    Button* m_btnInventory = nullptr;
    Button* m_btnSpells = nullptr;
    Button* m_btnQuests = nullptr;
    Button* m_btnSocial = nullptr;
    Button* m_btnOptions = nullptr;
    ProgressBar* m_xpBar = nullptr;
};
