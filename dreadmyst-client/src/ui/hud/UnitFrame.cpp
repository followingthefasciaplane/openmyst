#include "UnitFrame.h"
#include "ui/FontManager.h"
#include "core/Log.h"

void UnitFrame::init()
{
    // Position top-left with small margin
    setPosition(10.f, 10.f);
    setSize(372.f, 116.f);

    // Frame background
    auto frame = std::make_unique<ImageWidget>();
    frame->load("unit_frame.png");
    frame->setPosition(0.f, 0.f);
    m_frame = static_cast<ImageWidget*>(addChild(std::move(frame)));

    // Level background circle
    auto levelBg = std::make_unique<ImageWidget>();
    levelBg->load("unit_frame_level_bg.png");
    levelBg->setPosition(10.f, 75.f);
    m_levelBg = static_cast<ImageWidget*>(addChild(std::move(levelBg)));

    // Level label (centered on level bg)
    const sf::Font* font = sFontMgr.getBold();
    if (!font)
        font = sFontMgr.getDefault();

    if (font)
    {
        auto levelLabel = std::make_unique<Label>();
        levelLabel->init(*font, "1", 12);
        levelLabel->setColor(sf::Color::White);
        levelLabel->setPosition(20.f, 79.f);
        m_levelLabel = static_cast<Label*>(addChild(std::move(levelLabel)));

        // Name label
        auto nameLabel = std::make_unique<Label>();
        nameLabel->init(*font, "Player", 13);
        nameLabel->setColor(sf::Color::White);
        nameLabel->setPosition(60.f, 14.f);
        m_nameLabel = static_cast<Label*>(addChild(std::move(nameLabel)));
    }

    // HP bar - positioned inside frame
    auto hpBar = std::make_unique<ProgressBar>();
    hpBar->load("unit_frame_hp.png", "unit_frame_hp.png");
    hpBar->setPosition(60.f, 38.f);
    hpBar->setSize(296.f, 28.f);
    m_hpBar = static_cast<ProgressBar*>(addChild(std::move(hpBar)));

    // MP bar - positioned below HP bar
    auto mpBar = std::make_unique<ProgressBar>();
    mpBar->load("unit_frame_mp.png", "unit_frame_mp.png");
    mpBar->setPosition(80.f, 68.f);
    mpBar->setSize(265.f, 22.f);
    m_mpBar = static_cast<ProgressBar*>(addChild(std::move(mpBar)));

    LOG_INFO("UnitFrame initialized");
}

void UnitFrame::updateFromPlayer(const LocalPlayerData& player)
{
    if (m_hpBar && player.maxHealth > 0)
        m_hpBar->setValue(static_cast<float>(player.health) / static_cast<float>(player.maxHealth));

    if (m_mpBar && player.maxMana > 0)
        m_mpBar->setValue(static_cast<float>(player.mana) / static_cast<float>(player.maxMana));
    else if (m_mpBar)
        m_mpBar->setValue(0.f);

    if (m_levelLabel)
        m_levelLabel->setText(std::to_string(player.level));

    if (m_nameLabel && !player.name.empty())
        m_nameLabel->setText(player.name);
}
