#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdint>

namespace Util
{
    // Base64 encode (used for HTTP auth)
    inline std::string base64_encode(const std::string& input)
    {
        static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        int val = 0, valb = -6;
        for (unsigned char c : input)
        {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0)
            {
                result.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (result.size() % 4) result.push_back('=');
        return result;
    }

    // Format seconds into a human-readable duration string
    inline std::string formatTimeBySeconds(int totalSeconds)
    {
        int days = totalSeconds / 86400;
        int hours = (totalSeconds % 86400) / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;

        std::string result;
        if (days > 0) result += std::to_string(days) + "d ";
        if (hours > 0) result += std::to_string(hours) + "h ";
        if (minutes > 0) result += std::to_string(minutes) + "m ";
        result += std::to_string(seconds) + "s";
        return result;
    }

    // Split a string by delimiter
    inline std::vector<std::string> split(const std::string& str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::istringstream stream(str);
        std::string token;
        while (std::getline(stream, token, delimiter))
            tokens.push_back(token);
        return tokens;
    }

    // Trim whitespace
    inline std::string trim(const std::string& str)
    {
        auto start = str.find_first_not_of(" \t\r\n");
        auto end = str.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        return str.substr(start, end - start + 1);
    }

    // Case-insensitive string compare
    inline bool iequals(const std::string& a, const std::string& b)
    {
        return a.size() == b.size() &&
            std::equal(a.begin(), a.end(), b.begin(),
                [](char x, char y) { return tolower(x) == tolower(y); });
    }

    // Integer to string with thousands separator
    inline std::string formatNumber(int value)
    {
        std::string s = std::to_string(value);
        int n = (int)s.length() - 3;
        while (n > 0) { s.insert(n, ","); n -= 3; }
        return s;
    }

    // Convert string to lowercase
    inline std::string toLower(const std::string& str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
}
