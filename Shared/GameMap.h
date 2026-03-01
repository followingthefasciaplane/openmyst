#pragma once

#include "MapCellT.h"
#include "GameDefines.h"
#include <vector>
#include <map>
#include <memory>
#include <string>

class MapCellClient;

// Base class for game maps (shared between client and server).
// ClientMap inherits from this and adds rendering/input.
class GameMap
{
public:
    // Map layer/dimension defines
    enum Defines : int
    {
        BaseCellWidth   = 64,
        BaseCellHeight  = 32,
        Layer1          = 0,    // Ground layer
        Layer2          = 1,    // Lower objects
        Layer3          = 2,    // Upper objects / characters
        Layer4          = 3,    // Overhead / ceiling
        NumLayers       = 4,
    };

    virtual ~GameMap() = default;

    // Map dimensions
    int getMapWidth() const { return m_mapWidth; }
    int getMapHeight() const { return m_mapHeight; }
    int getTerrainWidth() const { return m_terrainWidth; }
    int getTerrainHeight() const { return m_terrainHeight; }
    int getMapId() const { return m_mapId; }

    void setMapDimensions(int width, int height) { m_mapWidth = width; m_mapHeight = height; }
    void setTerrainDimensions(int width, int height) { m_terrainWidth = width; m_terrainHeight = height; }
    void setMapId(int id) { m_mapId = id; }

    // Cell access
    MapCellT* getCell(int cellId) const;
    void setCell(int cellId, MapCellT* cell);

    int getTotalCells() const { return m_mapWidth * m_mapHeight; }

    // Loading callbacks (overridden by ClientMap/ServerMap)
    virtual void startedLoading() {}
    virtual void finishedLoading() {}
    virtual void onFinishedLoadingCells() {}
    virtual void onResize() {}
    virtual void onTerrainTextureLoaded(const int terrainId, const std::string& texture) {}
    virtual void onTerrainZoneLoaded(const int terrainId, const int zoneId) {}
    virtual void onTerrainAreaLoaded(const int terrainId, const int areaId) {}
    virtual void onCellDataLoaded(const int cellId, const int flags,
                                   const std::vector<std::shared_ptr<std::string>>& textures,
                                   const std::vector<float>& layerScale) {}

    // Cell position helpers
    static void cellIdToXY(int cellId, int mapWidth, int& x, int& y);
    static int  xyToCellId(int x, int y, int mapWidth);

protected:
    int m_mapId        = 0;
    int m_mapWidth     = 0;
    int m_mapHeight    = 0;
    int m_terrainWidth = 0;
    int m_terrainHeight = 0;

    std::map<int, MapCellT*> m_cells;
};
