#include "core/Config.h"
#include <fstream>
#include <algorithm>
#include <sstream>

Config& Config::instance() {
    static Config inst;
    return inst;
}

bool Config::load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_filename = filename;
    m_values.clear();

    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string currentSection;
    std::string line;

    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Skip comments
        if (line[0] == '#' || line[0] == ';') continue;

        // Section header
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                currentSection = line.substr(1, end - 1);
            }
            continue;
        }

        // Key=Value
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);

            // Trim key
            size_t kend = key.find_last_not_of(" \t");
            if (kend != std::string::npos) key = key.substr(0, kend + 1);

            // Trim value
            size_t vstart = value.find_first_not_of(" \t");
            if (vstart != std::string::npos) value = value.substr(vstart);
            size_t vend = value.find_last_not_of(" \t\r\n");
            if (vend != std::string::npos) value = value.substr(0, vend + 1);

            // Strip quotes
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            m_values[makeKey(currentSection, key)] = value;
        }
    }
    return true;
}

std::string Config::makeKey(const std::string& section, const std::string& key) const {
    return section + "." + key;
}

int Config::getInt(const std::string& section, const std::string& key, int defaultVal) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(makeKey(section, key));
    if (it == m_values.end()) return defaultVal;
    try { return std::stoi(it->second); }
    catch (...) { return defaultVal; }
}

std::string Config::getString(const std::string& section, const std::string& key,
                               const std::string& defaultVal) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(makeKey(section, key));
    if (it == m_values.end()) return defaultVal;
    return it->second;
}

float Config::getFloat(const std::string& section, const std::string& key, float defaultVal) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(makeKey(section, key));
    if (it == m_values.end()) return defaultVal;
    try { return std::stof(it->second); }
    catch (...) { return defaultVal; }
}

bool Config::getBool(const std::string& section, const std::string& key, bool defaultVal) const {
    return getInt(section, key, defaultVal ? 1 : 0) != 0;
}

void Config::setInt(const std::string& section, const std::string& key, int value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[makeKey(section, key)] = std::to_string(value);
}

void Config::setString(const std::string& section, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values[makeKey(section, key)] = value;
}
