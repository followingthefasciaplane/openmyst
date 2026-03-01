#pragma once

#include <cstdint>

namespace AccountDefines
{
    // Account access levels
    enum class AccessLevel : int
    {
        Player    = 0,
        Moderator = 1,
        Admin     = 2,
    };

    // Account ban status
    enum class BanStatus : int
    {
        None       = 0,
        Temporary  = 1,
        Permanent  = 2,
    };

    // Authentication result codes (from Server_Validate handler at 0x0045a940)
    // case 0 = success, cases 1-4 = error with TimedMessageBox
    enum class AuthenticateResult : int
    {
        Validated       = 0,
        BadPassword     = 1,
        ServerFull      = 2,
        Banned          = 3,
        WrongVersion    = 4,
    };
}
