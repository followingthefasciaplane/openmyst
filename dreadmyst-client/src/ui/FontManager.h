#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>

// FontManager - loads and caches game fonts.
// Binary uses Friz Quadrata Regular.ttf for most UI text,
// Friz Quadrata Bold.ttf for headings, arial.ttf as fallback.
// Font directory: assets/content/fonts/ (11 TTF files).

class FontManager
{
public:
    static FontManager& instance();

    // Load all .ttf files from the given directory
    bool init(const std::string& fontDir);

    // Get font by filename (without path), e.g. "arial.ttf"
    const sf::Font* getFont(const std::string& name) const;

    // Convenience accessors matching binary usage
    const sf::Font* getDefault() const;  // "Friz Quadrata Regular.ttf"
    const sf::Font* getBold() const;     // "Friz Quadrata Bold.ttf"

private:
    FontManager() = default;
    std::unordered_map<std::string, sf::Font> m_fonts;
    bool m_initialized = false;
};

#define sFontMgr FontManager::instance()
