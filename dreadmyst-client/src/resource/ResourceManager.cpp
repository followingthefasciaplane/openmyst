#include "ResourceManager.h"
#include "core/Log.h"

#include <zip.h>
#include <filesystem>
#include <fstream>

// ResourceManager implementation.
//
// Decompiled from ContentMgr in Dreadmyst.exe:
//   0x0043d170 - documentZips: iterates zip_get_num_entries, maps each filename to zip path
//   0x0043ecd0 - unzip: zip_open -> zip_stat -> zip_fopen -> zip_fread -> zip_fclose
//   0x0043d370 - getTexture: check cache, if miss: unzip -> sf::Texture::loadFromMemory -> cache

ResourceManager::ResourceManager()
{
    createFallbackTexture();
}

void ResourceManager::createFallbackTexture()
{
    // 1x1 magenta pixel as fallback for missing textures
    sf::Image img({1, 1}, sf::Color::Magenta);
    m_fallbackTexture = std::make_shared<sf::Texture>(img);
}

ResourceManager& ResourceManager::instance()
{
    static ResourceManager inst;
    return inst;
}

// Binary: ContentMgr::documentZips at 0x0043d170
// Scans all ZIP files in a directory, mapping each contained filename to its zip path.
void ResourceManager::scanDirectory(const std::string& dirPath)
{
    namespace fs = std::filesystem;

    if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
    {
        LOG_WARN("ResourceManager: directory '%s' not found", dirPath.c_str());
        return;
    }

    int totalFiles = 0;
    int totalZips = 0;

    for (const auto& entry : fs::recursive_directory_iterator(dirPath))
    {
        if (!entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();
        if (ext != ".zip")
            continue;

        std::string zipPath = entry.path().string();

        int err = 0;
        zip_t* z = zip_open(zipPath.c_str(), ZIP_RDONLY, &err);
        if (!z)
        {
            LOG_WARN("ResourceManager: unable to open zip '%s' (err=%d)", zipPath.c_str(), err);
            continue;
        }

        zip_int64_t numEntries = zip_get_num_entries(z, 0);
        for (zip_int64_t i = 0; i < numEntries; i++)
        {
            const char* name = zip_get_name(z, static_cast<zip_uint64_t>(i), 0);
            if (name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_fileToZip[name] = zipPath;
                totalFiles++;
            }
        }

        zip_close(z);
        totalZips++;
    }

    LOG_INFO("ResourceManager: indexed %d files from %d ZIPs in '%s'",
             totalFiles, totalZips, dirPath.c_str());
}

// Binary: ContentMgr::unzip at 0x0043ecd0
// Extracts a file from its ZIP archive. Caller must free() the returned data.
bool ResourceManager::extractFile(const std::string& filename, void** outData, size_t* outSize)
{
    std::string zipPath;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_fileToZip.find(filename);
        if (it == m_fileToZip.end())
            return false;
        zipPath = it->second;
    }

    int err = 0;
    zip_t* z = zip_open(zipPath.c_str(), ZIP_RDONLY, &err);
    if (!z)
    {
        LOG_ERROR("ResourceManager: unable to open zip '%s'", zipPath.c_str());
        return false;
    }

    zip_stat_t stat;
    zip_stat_init(&stat);
    if (zip_stat(z, filename.c_str(), 0, &stat) != 0)
    {
        zip_close(z);
        return false;
    }

    zip_file_t* f = zip_fopen(z, filename.c_str(), 0);
    if (!f)
    {
        zip_close(z);
        return false;
    }

    *outSize = static_cast<size_t>(stat.size);
    *outData = malloc(*outSize);

    zip_int64_t bytesRead = zip_fread(f, *outData, *outSize);
    zip_fclose(f);
    zip_close(z);

    if (bytesRead < 0 || static_cast<size_t>(bytesRead) != *outSize)
    {
        free(*outData);
        *outData = nullptr;
        *outSize = 0;
        return false;
    }

    return true;
}

// Binary: ContentMgr::getTexture at 0x0043d370
// Returns a cached texture or loads it from a ZIP archive.
std::shared_ptr<sf::Texture> ResourceManager::getTexture(const std::string& filename)
{
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_textureCache.find(filename);
        if (it != m_textureCache.end())
            return it->second;
    }

    // Extract from ZIP
    void* data = nullptr;
    size_t size = 0;
    if (!extractFile(filename, &data, &size))
    {
        LOG_WARN("ResourceManager: texture '%s' not found in any ZIP, using fallback", filename.c_str());
        return m_fallbackTexture;
    }

    // Create texture from memory
    auto tex = std::make_shared<sf::Texture>();
    if (!tex->loadFromMemory(data, size))
    {
        LOG_ERROR("ResourceManager: failed to load texture '%s' from memory, using fallback", filename.c_str());
        free(data);
        return m_fallbackTexture;
    }

    free(data);

    // Cache it
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_textureCache[filename] = tex;
    }

    return tex;
}

bool ResourceManager::loadFileFromDisk(const std::string& path, std::vector<uint8_t>& outData)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LOG_WARN("ResourceManager: file '%s' not found on disk", path.c_str());
        return false;
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    outData.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(outData.data()), size);

    return file.good();
}
