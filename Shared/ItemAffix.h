#pragma once

#include <cstdint>
#include <string>

// Item affix (prefix/suffix modifiers on generated items)
struct ItemAffix
{
    int         id          = 0;
    std::string name;
    int         statType    = 0;
    int         statValue   = 0;
    int         minLevel    = 0;

    bool isValid() const { return id > 0; }
};
