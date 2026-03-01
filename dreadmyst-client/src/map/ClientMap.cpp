#include "ClientMap.h"
#include "core/Log.h"

#include <cmath>
#include <algorithm>

// ClientMap implementation.
//
// Isometric constants from binary data sections:
//   TILE_HALF_WIDTH  = 32.0f  (0x00648a8c)
//   TILE_HALF_HEIGHT = 16.0f  (0x00648a54)
//   INV_TILE_WIDTH   = 1/64   (0x006488b0)
//   INV_TILE_HEIGHT  = 1/32   (0x006488c4)
//   TERRAIN_HALF_W   = 416.0f (0x00648b1c) = 32 * 13
//   TERRAIN_HALF_H   = 208.0f (0x00648b00) = 16 * 13

static constexpr float TILE_HALF_W = 32.0f;
static constexpr float TILE_HALF_H = 16.0f;
static constexpr float INV_TILE_W  = 1.0f / 64.0f;
static constexpr float INV_TILE_H  = 1.0f / 32.0f;
static constexpr float TERRAIN_HALF_W = 416.0f;
static constexpr float TERRAIN_HALF_H = 208.0f;

const std::string ClientMap::s_emptyString;

ClientMap::ClientMap()
{
}

bool ClientMap::loadFromFile(const MapFileData& mapData)
{
    m_mapWidth  = static_cast<int>(mapData.mapWidth);
    m_mapHeight = static_cast<int>(mapData.mapWidth); // square map
    m_terrainWidth  = m_mapWidth / 13;
    m_terrainHeight = m_mapWidth / 13;

    m_objectTextures  = mapData.objectTextures;
    m_terrainTextures = mapData.terrainTextures;

    // Load cells
    for (const auto& cellData : mapData.cells)
    {
        auto cell = std::make_unique<MapCellClient>();
        cell->setCellId(static_cast<int>(cellData.cellId));
        cell->setFlags(cellData.flags);

        for (int layer = 0; layer < 3; layer++)
        {
            if (cellData.layers[layer].hasTexture)
            {
                uint32_t texIdx = cellData.layers[layer].textureIndex;
                if (texIdx < m_objectTextures.size())
                {
                    cell->setLayer(layer, m_objectTextures[texIdx],
                                   cellData.layers[layer].scale);
                }
            }
        }

        m_clientCells[static_cast<int>(cellData.cellId)] = std::move(cell);
    }

    // Load terrain assignments
    for (const auto& ta : mapData.terrainAssignments)
    {
        if (ta.textureIndex < m_terrainTextures.size())
        {
            m_terrainMap[static_cast<int>(ta.terrainCellId)] =
                m_terrainTextures[ta.textureIndex];
        }
    }

    LOG_INFO("ClientMap: loaded %dx%d, %zu cells, terrain %dx%d",
             m_mapWidth, m_mapHeight, m_clientCells.size(),
             m_terrainWidth, m_terrainHeight);

    return true;
}

MapCellClient* ClientMap::getClientCell(int cellId) const
{
    auto it = m_clientCells.find(cellId);
    if (it != m_clientCells.end())
        return it->second.get();
    return nullptr;
}

const std::string& ClientMap::getTerrainTexture(int terrainCellId) const
{
    auto it = m_terrainMap.find(terrainCellId);
    if (it != m_terrainMap.end())
        return it->second;
    return s_emptyString;
}

// Binary: inline in FUN_004ad950 (render thread)
void ClientMap::getCellScreenPos(int cellId, float& outX, float& outY) const
{
    int row = cellId / m_mapWidth;
    int col = cellId % m_mapWidth;

    outX = (static_cast<float>(col) * TILE_HALF_W - static_cast<float>(row) * TILE_HALF_W)
           + std::floor(m_cameraX);
    outY = (static_cast<float>(row) * TILE_HALF_H + static_cast<float>(col) * TILE_HALF_H)
           + std::floor(m_cameraY);
}

// Binary: FUN_004af0c0
void ClientMap::getTerrainScreenPos(int terrainCellId, float& outX, float& outY) const
{
    int tw = m_mapWidth / 13;
    if (tw <= 0) { outX = outY = 0; return; }

    int row = terrainCellId / tw;
    int col = terrainCellId % tw;

    outX = (static_cast<float>(col) * TERRAIN_HALF_W - static_cast<float>(row) * TERRAIN_HALF_W)
           + std::floor(m_cameraX);
    outY = (static_cast<float>(row) * TERRAIN_HALF_H + static_cast<float>(col) * TERRAIN_HALF_H)
           + std::floor(m_cameraY);
}

// Binary: FUN_004aeff0
int ClientMap::screenToCell(float screenX, float screenY) const
{
    if (m_mapWidth <= 0) return 0;

    float camX = std::floor(m_cameraX);
    float camY = std::floor(m_cameraY);
    int maxIdx = m_mapWidth - 1;

    float relX = (screenX - camX) * INV_TILE_W;
    float relY = (screenY - camY) * INV_TILE_H;

    float colF = relY - relX;
    float rowF = relX + relY;

    int col = std::clamp(static_cast<int>(colF), 0, maxIdx);
    int row = std::clamp(static_cast<int>(rowF), 0, maxIdx);

    return col * m_mapWidth + row;
}

void ClientMap::onCellDataLoaded(const int cellId, const int flags,
                                  const std::vector<std::shared_ptr<std::string>>& textures,
                                  const std::vector<float>& layerScale)
{
    auto cell = std::make_unique<MapCellClient>();
    cell->setCellId(cellId);
    cell->setFlags(flags);

    for (size_t i = 0; i < textures.size() && i < 3; i++)
    {
        if (textures[i] && !textures[i]->empty())
        {
            float scale = (i < layerScale.size()) ? layerScale[i] : 1.0f;
            cell->setLayer(static_cast<int>(i), *textures[i], scale);
        }
    }

    m_clientCells[cellId] = std::move(cell);
}
