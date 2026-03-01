#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <cstdint>

// ResourceManager - ZIP extraction and texture caching.
//
// Decompiled from ContentMgr in binary:
//   0x0043d170 - documentZips (scan ZIP contents)
//   0x0043ecd0 - unzip (extract file from ZIP)
//   0x0043d370 - getTexture (load + cache texture)
//   0x0043e1c0 - spawnSprite (create sprite from texture)
//
// Binary ContentMgr scans all ZIPs in content/ directory on startup,
// building a filename -> zipPath mapping. When a texture is needed,
// it looks up the zip path, opens the zip, extracts the file,
// creates sf::Texture from memory, and caches it.

class ResourceManager
{
public:
    static ResourceManager& instance();

    // Scan a directory for ZIP files and index their contents.
    // Binary: ContentMgr::documentZips at 0x0043d170
    void scanDirectory(const std::string& dirPath);

    // Extract a file from its ZIP archive into a memory buffer.
    // Binary: ContentMgr::unzip at 0x0043ecd0
    // Returns true on success. Caller must free() the returned data.
    bool extractFile(const std::string& filename, void** outData, size_t* outSize);

    // Get or load a texture by filename. Cached after first load.
    // Binary: ContentMgr::getTexture at 0x0043d370
    std::shared_ptr<sf::Texture> getTexture(const std::string& filename);

    // Load raw file data from disk (not from ZIP).
    bool loadFileFromDisk(const std::string& path, std::vector<uint8_t>& outData);

private:
    ResourceManager();
    ~ResourceManager() = default;

    void createFallbackTexture();

    std::mutex m_mutex;

    // filename -> zip file path mapping
    std::unordered_map<std::string, std::string> m_fileToZip;

    // texture cache: filename -> shared_ptr<sf::Texture>
    std::unordered_map<std::string, std::shared_ptr<sf::Texture>> m_textureCache;

    // 1x1 magenta fallback texture for missing assets
    std::shared_ptr<sf::Texture> m_fallbackTexture;
};

#define sResourceMgr ResourceManager::instance()
