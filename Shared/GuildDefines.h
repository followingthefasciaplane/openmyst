#pragma once

#include <cstdint>
#include <string>

namespace GuildDefines
{
    // Guild rank levels (from Server_GuildNotifyRoleChange handler at 0x00464e00)
    // switch: 0="Initiate", 1="Member", 2="Veteran", 3="Officer", 4="Leader"
    enum class Rank : uint8_t
    {
        Initiate    = 0,
        Member      = 1,
        Veteran     = 2,
        Officer     = 3,
        Leader      = 4,
    };

    // Guild roster member entry (from Server_GuildRoster at 0x00466940)
    // Internal struct size: 0x24 (36 bytes)
    // Fields: uint32 charId, uint8 rank, uint8 level, uint8 classId, string name, bool online
    struct MemberInfo
    {
        uint32_t    m_characterId = 0;
        std::string m_name;
        uint8_t     m_level   = 0;
        uint8_t     m_classId = 0;
        Rank        m_rank    = Rank::Initiate;
        bool        m_online  = false;
    };
}
