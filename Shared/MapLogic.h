#pragma once

#include "GameDefines.h"
#include "MapCellT.h"
#include <vector>

class GameMap;

// Static pathfinding and spatial query utilities shared between client and server.
namespace MapLogic
{
    // Get cell IDs in a square region around a center cell
    void getIndexesAround(std::vector<int>& output, int centerCellId, int mapWidth, int searchSize);

    // Line-of-sight check between two world positions
    // Returns true if there is clear LOS (no blocking cells of the given flag type)
    bool checkLosToC(const GameMap& map, const Geo2d::Vector2& from, const Geo2d::Vector2& to, int cellFlag);

    // A* pathfinding from start to end, populating the path vector
    void constructPathTo(const GameMap& map, std::vector<Geo2d::Vector2>& path,
                         const Geo2d::Vector2& start, const Geo2d::Vector2& end);

    // Calculate total length of a path (sum of segment distances)
    float getPathLength(const Geo2d::Vector2& start, const std::vector<Geo2d::Vector2>& path);

    // Convert world position to cell ID
    int worldPosToCellId(float worldX, float worldY, int mapWidth);

    // Convert cell ID to world position (center of cell)
    Geo2d::Vector2 cellIdToWorldPos(int cellId, int mapWidth);
}
