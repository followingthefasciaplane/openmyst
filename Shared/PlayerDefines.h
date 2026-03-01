#pragma once

#include <cstdint>

namespace PlayerDefines
{
    // Player character classes (confirmed from binary string data at 0x642508+)
    enum class Classes : int
    {
        None    = 0,
        Paladin = 1,
        Ranger  = 2,
        Mage    = 3,
        Cleric  = 4,
        Max     = 5,
    };

    // Player gender
    enum class Gender : int
    {
        Male   = 0,
        Female = 1,
    };

    inline const char* classToString(Classes c)
    {
        switch (c) {
            case Classes::Paladin: return "Paladin";
            case Classes::Ranger:  return "Ranger";
            case Classes::Mage:    return "Mage";
            case Classes::Cleric:  return "Cleric";
            default:               return "Unknown Class";
        }
    }

    inline const char* genderToString(Gender g)
    {
        switch (g) {
            case Gender::Male:   return "male";
            case Gender::Female: return "female";
            default:             return "unknown";
        }
    }
}
