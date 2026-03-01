#include "FontManager.h"
#include "core/Log.h"
#include <filesystem>

FontManager& FontManager::instance()
{
    static FontManager inst;
    return inst;
}

bool FontManager::init(const std::string& fontDir)
{
    if (m_initialized)
        return true;

    namespace fs = std::filesystem;

    if (!fs::exists(fontDir) || !fs::is_directory(fontDir))
    {
        LOG_WARN("FontManager: font directory '%s' not found", fontDir.c_str());
        return false;
    }

    int count = 0;
    for (const auto& entry : fs::directory_iterator(fontDir))
    {
        if (!entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();
        // Case-insensitive .ttf check
        if (ext != ".ttf" && ext != ".TTF")
            continue;

        std::string filename = entry.path().filename().string();
        std::string fullPath = entry.path().string();

        sf::Font font;
        if (!font.openFromFile(fullPath))
        {
            LOG_WARN("FontManager: failed to load '%s'", fullPath.c_str());
            continue;
        }

        m_fonts[filename] = std::move(font);
        count++;
    }

    m_initialized = true;
    LOG_INFO("FontManager: loaded %d fonts from '%s'", count, fontDir.c_str());
    return count > 0;
}

const sf::Font* FontManager::getFont(const std::string& name) const
{
    auto it = m_fonts.find(name);
    if (it != m_fonts.end())
        return &it->second;
    return nullptr;
}

const sf::Font* FontManager::getDefault() const
{
    return getFont("Friz Quadrata Regular.ttf");
}

const sf::Font* FontManager::getBold() const
{
    return getFont("Friz Quadrata Bold.ttf");
}
