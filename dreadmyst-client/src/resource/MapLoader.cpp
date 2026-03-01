#include "MapLoader.h"
#include "core/Log.h"

#include <fstream>
#include <cstring>

// MapLoader implementation.
//
// Decompiled from GameMap::loadFromDisk at 0x00555fc0 in Dreadmyst.exe.
// File format verified against all 28 .map files -- parses to exact EOF.

namespace
{

// Simple cursor-based reader for the map byte buffer.
struct MapReader
{
    const uint8_t* data;
    size_t size;
    size_t pos = 0;

    bool hasBytes(size_t n) const { return pos + n <= size; }

    uint8_t readU8()
    {
        uint8_t v = data[pos];
        pos += 1;
        return v;
    }

    uint32_t readU32()
    {
        uint32_t v;
        memcpy(&v, data + pos, 4);
        pos += 4;
        return v;
    }

    float readFloat()
    {
        float v;
        memcpy(&v, data + pos, 4);
        pos += 4;
        return v;
    }

    // Read a null-terminated string.
    std::string readNullString()
    {
        const char* start = reinterpret_cast<const char*>(data + pos);
        size_t len = strnlen(start, size - pos);
        pos += len + 1; // skip null terminator
        return std::string(start, len);
    }
};

} // anonymous namespace

bool MapLoader::load(const std::string& filePath, MapFileData& outData)
{
    // Read entire file into memory
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LOG_ERROR("MapLoader: cannot open '%s'", filePath.c_str());
        return false;
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));
    file.close();

    MapReader r;
    r.data = buffer.data();
    r.size = fileSize;

    // 1. Map width (binary: mapSize, square map)
    if (!r.hasBytes(4)) return false;
    outData.mapWidth = r.readU32();

    if (outData.mapWidth > 2000)
    {
        LOG_ERROR("MapLoader: map too big (%u > 2000)", outData.mapWidth);
        return false;
    }

    // 2. Number of object textures
    if (!r.hasBytes(4)) return false;
    uint32_t numObjTextures = r.readU32();

    // 3. Object texture strings (null-terminated)
    outData.objectTextures.reserve(numObjTextures);
    for (uint32_t i = 0; i < numObjTextures; i++)
    {
        if (r.pos >= r.size) return false;
        outData.objectTextures.push_back(r.readNullString());
    }

    // 4. Number of cells
    if (!r.hasBytes(4)) return false;
    uint32_t numCells = r.readU32();

    // 5. Cell data
    outData.cells.reserve(numCells);
    for (uint32_t i = 0; i < numCells; i++)
    {
        if (!r.hasBytes(5)) return false; // cellId(4) + flags(1)

        MapCellData cell;
        cell.cellId = r.readU32();
        cell.flags  = r.readU8();

        // 3 layers
        for (int layer = 0; layer < 3; layer++)
        {
            if (!r.hasBytes(1)) return false;
            uint8_t hasTex = r.readU8();
            cell.layers[layer].hasTexture = (hasTex != 0);

            if (hasTex != 0)
            {
                if (!r.hasBytes(8)) return false; // textureIndex(4) + scale(4)
                cell.layers[layer].textureIndex = r.readU32();
                cell.layers[layer].scale = r.readFloat();

                if (cell.layers[layer].textureIndex >= numObjTextures)
                {
                    LOG_ERROR("MapLoader: cell %u layer %d has invalid texture index %u (max %u)",
                              cell.cellId, layer, cell.layers[layer].textureIndex, numObjTextures);
                    return false;
                }
            }
        }

        outData.cells.push_back(cell);
    }

    // 6. Number of terrain textures
    if (!r.hasBytes(4))
    {
        // Some maps end here -- that's OK per binary behavior
        return true;
    }
    uint32_t numTerrainTextures = r.readU32();

    if (numTerrainTextures > 0)
    {
        // 7. Terrain texture strings
        outData.terrainTextures.reserve(numTerrainTextures);
        for (uint32_t i = 0; i < numTerrainTextures; i++)
        {
            if (r.pos >= r.size) return false;
            outData.terrainTextures.push_back(r.readNullString());
        }

        // 8. Terrain assignments
        if (!r.hasBytes(4)) return true;
        uint32_t numTerrainAssign = r.readU32();
        outData.terrainAssignments.reserve(numTerrainAssign);
        for (uint32_t i = 0; i < numTerrainAssign; i++)
        {
            if (!r.hasBytes(8)) return false;
            TerrainAssignment ta;
            ta.terrainCellId = r.readU32();
            ta.textureIndex  = r.readU32();
            outData.terrainAssignments.push_back(ta);
        }
    }

    // 9. Zone assignments (tolerates EOF)
    if (r.hasBytes(4))
    {
        uint32_t numZones = r.readU32();
        outData.zoneAssignments.reserve(numZones);
        for (uint32_t i = 0; i < numZones; i++)
        {
            if (!r.hasBytes(8)) break;
            ZoneAssignment za;
            za.terrainId = r.readU32();
            za.zoneId    = r.readU32();
            outData.zoneAssignments.push_back(za);
        }
    }

    // 10. Area assignments (tolerates EOF)
    if (r.hasBytes(4))
    {
        uint32_t numAreas = r.readU32();
        outData.areaAssignments.reserve(numAreas);
        for (uint32_t i = 0; i < numAreas; i++)
        {
            if (!r.hasBytes(8)) break;
            AreaAssignment aa;
            aa.terrainId = r.readU32();
            aa.areaId    = r.readU32();
            outData.areaAssignments.push_back(aa);
        }
    }

    LOG_INFO("MapLoader: loaded '%s' - %ux%u, %zu cells, %zu obj textures, %zu terrain textures",
             filePath.c_str(), outData.mapWidth, outData.mapWidth,
             outData.cells.size(), outData.objectTextures.size(),
             outData.terrainTextures.size());

    return true;
}
