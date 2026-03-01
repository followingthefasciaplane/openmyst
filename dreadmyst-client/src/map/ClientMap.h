#pragma once

#include "GameMap.h"
#include "MapCell.h"
#include "resource/MapLoader.h"
#include "resource/ResourceManager.h"

#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

// ClientMap - client-side map with texture data and rendering support.
//
// Decompiled from ClientMap in binary:
//   Constructor: 0x004AA430 (RTTI: .?AVClientMap@@ at 0x00680660)
//   ensureCellExists: 0x004AEAF0
//   onCellDataLoaded: 0x00639A5C (string ref)
//   saveToDisk: 0x004B0670
//
// RTTI inheritance chain (from BaseClassDescriptors):
//   ClientMap -> GameMap (offset 0)
//   ClientMap -> RenderObject (offset 0x44)
//   ClientMap -> MouseableNode (offset 0x44)
//   ClientMap -> AnchoredPosition (offset 0x5C)
//   ClientMap -> DraggableNode (offset 0x80)
//
// Camera is stored in the map object:
//   Offset 0x118: float cameraX
//   Offset 0x11C: float cameraY

class ClientMap : public GameMap
{
public:
    ClientMap();
    ~ClientMap() override = default;

    // Load map from parsed file data
    bool loadFromFile(const MapFileData& mapData);

    // Get cell at cellId (can be null for empty cells)
    MapCellClient* getClientCell(int cellId) const;

    // Terrain texture for a terrain cell
    const std::string& getTerrainTexture(int terrainCellId) const;
    int getTerrainWidth() const { return m_mapWidth / 13; }

    // Camera position (binary offsets 0x118, 0x11C)
    float getCameraX() const { return m_cameraX; }
    float getCameraY() const { return m_cameraY; }
    void setCameraPosition(float x, float y) { m_cameraX = x; m_cameraY = y; }

    // Isometric coordinate conversion
    // Binary: getCellRenderPos inline in FUN_004ad950
    //   screenX = (col * 32 - row * 32) + floor(cameraX)
    //   screenY = (row * 16 + col * 16) + floor(cameraY)
    void getCellScreenPos(int cellId, float& outX, float& outY) const;

    // Terrain-level isometric conversion (13x13 cells per terrain tile)
    // Binary: FUN_004af0c0
    //   screenX = (col * 416 - row * 416) + floor(cameraX)
    //   screenY = (row * 208 + col * 208) + floor(cameraY)
    void getTerrainScreenPos(int terrainCellId, float& outX, float& outY) const;

    // Inverse: screen position to cell ID
    // Binary: FUN_004aeff0
    int screenToCell(float screenX, float screenY) const;

    // Object texture list
    const std::vector<std::string>& getObjectTextures() const { return m_objectTextures; }

    // GameMap virtual overrides
    void onCellDataLoaded(const int cellId, const int flags,
                          const std::vector<std::shared_ptr<std::string>>& textures,
                          const std::vector<float>& layerScale) override;

private:
    // Cell storage (indexed by cellId)
    std::unordered_map<int, std::unique_ptr<MapCellClient>> m_clientCells;

    // Texture name lists from the map file
    std::vector<std::string> m_objectTextures;
    std::vector<std::string> m_terrainTextures;

    // Terrain cell -> texture name mapping
    std::unordered_map<int, std::string> m_terrainMap;

    // Camera position
    float m_cameraX = 0.0f;
    float m_cameraY = 0.0f;

    static const std::string s_emptyString;
};
