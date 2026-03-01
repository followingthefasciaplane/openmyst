#include "Config.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// Config implementation verified against binary's FUN_004074a0 (initConfig)
// and FUN_00551d50 (WritePrivateProfileInt) / FUN_00552090 (GetPrivateProfileString).
//
// The binary uses Win32 GetPrivateProfileIntA / GetPrivateProfileStringA
// which operate on standard INI files: [Section] Key=Value format.
// We reimplement this as a cross-platform INI parser.

Config* Config::instance()
{
    static Config inst;
    return &inst;
}

static std::string trim(const std::string& s)
{
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string stripQuotes(const std::string& s)
{
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

bool Config::load(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_filename = filename;

    std::ifstream file(filename);
    if (!file.is_open())
        return false;

    std::string line;
    std::string currentSection;

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

        if (line.front() == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        auto eqPos = line.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = trim(line.substr(0, eqPos));
            std::string value = trim(line.substr(eqPos + 1));
            value = stripQuotes(value);

            m_values[makeKey(currentSection, key)] = value;
        }
    }

    return true;
}

int Config::getInt(const std::string& section, const std::string& key, int defaultVal) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(makeKey(section, key));
    if (it == m_values.end())
        return defaultVal;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultVal;
    }
}

std::string Config::getString(const std::string& section, const std::string& key,
                              const std::string& defaultVal) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(makeKey(section, key));
    if (it == m_values.end())
        return defaultVal;
    return it->second;
}

float Config::getFloat(const std::string& section, const std::string& key, float defaultVal) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(makeKey(section, key));
    if (it == m_values.end())
        return defaultVal;
    try {
        return std::stof(it->second);
    } catch (...) {
        return defaultVal;
    }
}

bool Config::getBool(const std::string& section, const std::string& key, bool defaultVal) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(makeKey(section, key));
    if (it == m_values.end())
        return defaultVal;

    const std::string& v = it->second;
    if (v == "1" || v == "true" || v == "True" || v == "TRUE")
        return true;
    if (v == "0" || v == "false" || v == "False" || v == "FALSE")
        return false;
    return defaultVal;
}

void Config::setInt(const std::string& section, const std::string& key, int value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[makeKey(section, key)] = std::to_string(value);
}

void Config::setString(const std::string& section, const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[makeKey(section, key)] = value;
}

std::string Config::makeKey(const std::string& section, const std::string& key) const
{
    return section + "." + key;
}
