#pragma once

#include <cstdint>
#include <string>
#include "PlayerDefines.h"

namespace CharacterDefines
{
    // Maximum characters per account
    constexpr int MAX_CHARACTERS = 5;

    // Character list entry (from SMSG_CHARACTER_LIST)
    struct CharacterInfo
    {
        int                    guid      = 0;
        std::string            name;
        int                    level     = 0;
        int                    mapId     = 0;
        PlayerDefines::Classes classId   = PlayerDefines::Classes::None;
        PlayerDefines::Gender  gender    = PlayerDefines::Gender::Male;
        int                    portrait  = 0;
    };
}
