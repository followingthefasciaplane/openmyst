#pragma once

#include <string>
#include <vector>
#include <cstdint>

// MapLoader - parses .map binary files.
//
// Decompiled from GameMap::loadFromDisk at 0x00555fc0.
// Cross-validated with ClientMap::saveToDisk at 0x004b0670.
//
// File format (sequential reads from byte buffer):
//   1. u32 mapWidth (square map: height = width, max 2000)
//   2. u32 numObjectTextures
//   3. numObjectTextures null-terminated strings
//   4. u32 numCells
//   5. Per cell: u32 cellId, u8 flags, 3x layers:
//      - u8 hasTexture, if >0: u32 textureIndex + f32 layerScale
//   6. u32 numTerrainTextures
//   7. numTerrainTextures null-terminated strings (if count > 0)
//   8. u32 numTerrainAssignments + [u32 terrainCellId, u32 textureIndex] (if terrain textures > 0)
//   9. u32 numZoneAssignments + [u32 terrainId, u32 zoneId]
//  10. u32 numAreaAssignments + [u32 terrainId, u32 areaId]

struct MapCellLayer
{
    bool   hasTexture = false;
    uint32_t textureIndex = 0;
    float  scale = 1.0f;
};

struct MapCellData
{
    uint32_t cellId = 0;
    uint8_t  flags  = 0;
    MapCellLayer layers[3];
};

struct TerrainAssignment
{
    uint32_t terrainCellId = 0;
    uint32_t textureIndex  = 0;
};

struct ZoneAssignment
{
    uint32_t terrainId = 0;
    uint32_t zoneId    = 0;
};

struct AreaAssignment
{
    uint32_t terrainId = 0;
    uint32_t areaId    = 0;
};

struct MapFileData
{
    uint32_t mapWidth = 0;

    std::vector<std::string> objectTextures;
    std::vector<MapCellData> cells;

    std::vector<std::string> terrainTextures;
    std::vector<TerrainAssignment> terrainAssignments;
    std::vector<ZoneAssignment> zoneAssignments;
    std::vector<AreaAssignment> areaAssignments;
};

namespace MapLoader
{
    // Load and parse a .map file from disk.
    // Returns true on success, populating outData.
    bool load(const std::string& filePath, MapFileData& outData);
}
