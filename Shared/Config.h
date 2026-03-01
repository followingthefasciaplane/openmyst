#pragma once

#include <string>
#include <map>
#include <mutex>

// INI-style configuration reader
// Reads from config.ini using [Section] Key=Value format
class Config {
public:
    static Config* instance();

    bool load(const std::string& filename);

    int getInt(const std::string& section, const std::string& key, int defaultVal = 0) const;
    std::string getString(const std::string& section, const std::string& key,
                          const std::string& defaultVal = "") const;
    float getFloat(const std::string& section, const std::string& key, float defaultVal = 0.0f) const;
    bool getBool(const std::string& section, const std::string& key, bool defaultVal = false) const;

    void setInt(const std::string& section, const std::string& key, int value);
    void setString(const std::string& section, const std::string& key, const std::string& value);

private:
    Config() = default;
    std::string makeKey(const std::string& section, const std::string& key) const;

    std::map<std::string, std::string> m_values;
    mutable std::mutex m_mutex;
    std::string m_filename;
};

#define sConfig Config::instance()

// Convenience accessor using pointer syntax: sConfig->getString(...)
