#pragma once

#include <cstdint>

namespace ChatDefines
{
    // Chat channel types (from Server_ChatMsg handler at 0x0045ccb0)
    // switch on chatType byte: 0=Say, 1=Party, 2=Whisper, 4=Guild, 6=System/GM, 9=Channel
    enum class Channels : int
    {
        Say       = 0,
        Party     = 1,
        Whisper   = 2,
        Guild     = 4,
        Emote     = 5,
        System    = 6,   // GM/system messages, blocked if chat disabled flag set
        Combat    = 7,
        Announce  = 8,
        Channel   = 9,   // Numbered public channels (General, Trade, etc.)
    };
}
