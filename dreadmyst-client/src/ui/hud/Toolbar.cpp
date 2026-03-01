#include "Toolbar.h"
#include "core/Log.h"

void Toolbar::init(unsigned int screenWidth, unsigned int screenHeight)
{
    // toolbar_base is 861x69
    float baseW = 861.f;
    float baseH = 69.f;
    setSize(baseW, baseH);

    // Center at bottom of screen
    reposition(screenWidth, screenHeight);

    // Base background
    auto base = std::make_unique<ImageWidget>();
    base->load("toolbar_base.png");
    base->setPosition(0.f, 0.f);
    m_base = static_cast<ImageWidget*>(addChild(std::move(base)));

    // XP bar - centered on toolbar, positioned at top
    auto xpBar = std::make_unique<ProgressBar>();
    xpBar->load("xp_bar.png", "xp_bar.png");
    float xpW = 633.f;
    xpBar->setPosition((baseW - xpW) / 2.f, 2.f);
    xpBar->setSize(xpW, 12.f);
    m_xpBar = static_cast<ProgressBar*>(addChild(std::move(xpBar)));

    // Menu buttons (36x36 each) - arranged right side of toolbar
    // Binary lays them out in a row: character, inventory, spells, quests, social, options
    float btnY = 22.f;
    float btnStartX = baseW - 6 * 40.f - 20.f;  // 6 buttons with spacing

    auto makeBtn = [&](const std::string& name, float x) -> Button*
    {
        auto btn = std::make_unique<Button>();
        btn->load("toolbar_" + name);
        btn->setPosition(x, btnY);
        btn->setOnClick([name]() { LOG_INFO("Toolbar: %s clicked", name.c_str()); });
        return static_cast<Button*>(addChild(std::move(btn)));
    };

    m_btnCharacter = makeBtn("character", btnStartX);
    m_btnInventory = makeBtn("inventory", btnStartX + 40.f);
    m_btnSpells    = makeBtn("spells",    btnStartX + 80.f);
    m_btnQuests    = makeBtn("quests",    btnStartX + 120.f);
    m_btnSocial    = makeBtn("social",    btnStartX + 160.f);
    m_btnOptions   = makeBtn("options",   btnStartX + 200.f);

    LOG_INFO("Toolbar initialized");
}

void Toolbar::updateXP(uint32_t xp, uint32_t xpToLevel)
{
    if (m_xpBar && xpToLevel > 0)
        m_xpBar->setValue(static_cast<float>(xp) / static_cast<float>(xpToLevel));
}

void Toolbar::reposition(unsigned int screenWidth, unsigned int screenHeight)
{
    float x = (static_cast<float>(screenWidth) - m_width) / 2.f;
    float y = static_cast<float>(screenHeight) - m_height - 5.f;
    setPosition(x, y);
}
