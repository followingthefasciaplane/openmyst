#pragma once

#include "ui/Widget.h"
#include "ui/ImageWidget.h"
#include "ui/ProgressBar.h"
#include "ui/Label.h"
#include "network/PacketHandler.h"

// UnitFrame - player HP/MP display in top-left corner.
// Matches binary's UnitFrame class.
// Shows player portrait area, HP bar, MP bar, level.
//
// Texture layout (from unit_frame.png 372x116):
//   - Frame background: 372x116
//   - HP bar (296x28): positioned inside frame
//   - MP bar (265x22): positioned below HP bar
//   - Level bg (33x33): positioned at left side

class UnitFrame : public Widget
{
public:
    void init();
    void updateFromPlayer(const LocalPlayerData& player);

private:
    ImageWidget* m_frame = nullptr;
    ProgressBar* m_hpBar = nullptr;
    ProgressBar* m_mpBar = nullptr;
    Label* m_nameLabel = nullptr;
    Label* m_levelLabel = nullptr;
    ImageWidget* m_levelBg = nullptr;
};
