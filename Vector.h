#pragma once

#include <vector>
#include <algorithm>
#include <string>

namespace Util
{
    // Remove an element from a vector by value
    template<typename T>
    inline bool vectorRemove(std::vector<T>& vec, const T& value)
    {
        auto it = std::find(vec.begin(), vec.end(), value);
        if (it != vec.end()) { vec.erase(it); return true; }
        return false;
    }

    // Check if vector contains a value
    template<typename T>
    inline bool vectorContains(const std::vector<T>& vec, const T& value)
    {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }

    // Join a vector of strings with a delimiter
    inline std::string vectorJoin(const std::vector<std::string>& vec, const std::string& delimiter)
    {
        std::string result;
        for (size_t i = 0; i < vec.size(); ++i)
        {
            if (i > 0) result += delimiter;
            result += vec[i];
        }
        return result;
    }
}
