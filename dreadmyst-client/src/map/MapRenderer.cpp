#include "MapRenderer.h"
#include "ClientMap.h"
#include "MapCell.h"
#include "resource/ResourceManager.h"
#include "core/Log.h"

#include <cmath>
#include <algorithm>

// MapRenderer implementation.
//
// Decompiled from GameMap::renderThread at 0x004ad950.
// Binary visibility expansion: (maxScreenDim / 64) + 14 cells margin.
// Binary terrain expansion: (maxScreenDim / 832) + 2 terrain cells margin.
//
// Render order: terrain first, then cell layers 0, 1, 2.
// Cells are drawn in row-major order (back-to-front for isometric).

static constexpr float TILE_HALF_W = 32.0f;
static constexpr float TILE_HALF_H = 16.0f;
static constexpr float TILE_FULL_W = 64.0f;
static constexpr float TILE_FULL_H = 32.0f;
static constexpr int   RENDER_MARGIN = 14;
static constexpr int   TERRAIN_MARGIN = 2;

void MapRenderer::render(sf::RenderTarget& target, ClientMap& map,
                         unsigned int screenWidth, unsigned int screenHeight)
{
    if (map.getMapWidth() <= 0)
        return;

    // One-shot debug log on first render
    if (!m_debugLogged)
    {
        m_debugLogged = true;
        float camX = map.getCameraX();
        float camY = map.getCameraY();
        LOG_INFO("MapRenderer: FIRST RENDER - mapW=%d, screen=%ux%u, camera=(%.1f, %.1f)",
                 map.getMapWidth(), screenWidth, screenHeight, camX, camY);
    }

    // Draw terrain layer first (large background tiles)
    renderTerrain(target, map, screenWidth, screenHeight);

    // Draw cell layers in order: ground, lower objects, upper objects
    renderCellLayer(target, map, 0, screenWidth, screenHeight);
    renderCellLayer(target, map, 1, screenWidth, screenHeight);
    renderCellLayer(target, map, 2, screenWidth, screenHeight);
}

void MapRenderer::renderTerrain(sf::RenderTarget& target, ClientMap& map,
                                unsigned int screenWidth, unsigned int screenHeight)
{
    int tw = map.getTerrainWidth();
    if (tw <= 0) return;

    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);

    // Determine visible terrain cell range
    // Binary: screenToTerrainCell from screen center, expand by margin
    float centerX = sw * 0.5f;
    float centerY = sh * 0.5f;

    float camX = std::floor(map.getCameraX());
    float camY = std::floor(map.getCameraY());

    // Inverse isometric for terrain (416/208 per terrain cell)
    float relX = (centerX - camX) / (TILE_FULL_W * 13.0f);
    float relY = (centerY - camY) / (TILE_FULL_H * 13.0f);
    float colF = relY - relX;
    float rowF = relX + relY;

    int maxDim = std::max(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
    int margin = maxDim / 832 + TERRAIN_MARGIN;

    int startCol = std::max(0, static_cast<int>(colF) - margin);
    int startRow = std::max(0, static_cast<int>(rowF) - margin);
    int endCol   = std::min(tw - 1, static_cast<int>(colF) + margin);
    int endRow   = std::min(tw - 1, static_cast<int>(rowF) + margin);

    int terrainTotal = 0;
    int terrainVisible = 0;
    int terrainLoaded = 0;

    for (int row = startRow; row <= endRow; row++)
    {
        for (int col = startCol; col <= endCol; col++)
        {
            int terrainId = row * tw + col;
            const std::string& texName = map.getTerrainTexture(terrainId);
            if (texName.empty()) continue;
            terrainTotal++;

            float sx, sy;
            map.getTerrainScreenPos(terrainId, sx, sy);

            // Frustum check
            if (sx + 832.0f < 0.0f || sx > sw || sy + 416.0f < 0.0f || sy > sh)
                continue;
            terrainVisible++;

            auto tex = sResourceMgr.getTexture(texName);
            if (!tex) continue;
            terrainLoaded++;

            sf::Sprite sprite(*tex);
            sprite.setPosition({sx, sy});
            target.draw(sprite);
        }
    }

    if (!m_terrainDebugLogged)
    {
        m_terrainDebugLogged = true;
        LOG_INFO("MapRenderer::renderTerrain: range=[%d..%d][%d..%d], cam=(%.1f,%.1f), found=%d visible=%d loaded=%d",
                 startRow, endRow, startCol, endCol, camX, camY, terrainTotal, terrainVisible, terrainLoaded);
    }
}

void MapRenderer::renderCellLayer(sf::RenderTarget& target, ClientMap& map,
                                  int layer, unsigned int screenWidth, unsigned int screenHeight)
{
    int mapW = map.getMapWidth();
    if (mapW <= 0) return;

    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);

    // Determine visible cell range from screen center
    float centerX = sw * 0.5f;
    float centerY = sh * 0.5f;

    float camX = std::floor(map.getCameraX());
    float camY = std::floor(map.getCameraY());

    // Inverse isometric for cells
    float relX = (centerX - camX) / TILE_FULL_W;
    float relY = (centerY - camY) / TILE_FULL_H;
    float colF = relY - relX;
    float rowF = relX + relY;

    int maxDim = std::max(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
    int margin = maxDim / 64 + RENDER_MARGIN;

    int startCol = std::max(0, static_cast<int>(colF) - margin);
    int startRow = std::max(0, static_cast<int>(rowF) - margin);
    int endCol   = std::min(mapW - 1, static_cast<int>(colF) + margin);
    int endRow   = std::min(mapW - 1, static_cast<int>(rowF) + margin);

    int cellTotal = 0;
    int cellVisible = 0;
    int cellLoaded = 0;

    // Iterate in row-major order (isometric back-to-front)
    for (int row = startRow; row <= endRow; row++)
    {
        for (int col = startCol; col <= endCol; col++)
        {
            int cellId = row * mapW + col;
            MapCellClient* cell = map.getClientCell(cellId);
            if (!cell) continue;

            const CellLayer& cl = cell->getLayer(layer);
            if (cl.textureName.empty()) continue;
            cellTotal++;

            float sx, sy;
            map.getCellScreenPos(cellId, sx, sy);

            // Frustum check with generous bounds for large textures
            if (sx + 256.0f < 0.0f || sx - 128.0f > sw ||
                sy + 256.0f < 0.0f || sy - 128.0f > sh)
                continue;
            cellVisible++;

            // Lazy-load texture
            if (!cl.loaded)
            {
                auto& mutableLayer = cell->getLayer(layer);
                mutableLayer.texture = sResourceMgr.getTexture(cl.textureName);
                mutableLayer.loaded = true;
            }

            if (!cl.texture) continue;
            cellLoaded++;

            sf::Sprite sprite(*cl.texture);
            sprite.setPosition({sx, sy});

            // Apply scale (negative = flipped horizontally)
            float scale = cl.scale;
            if (scale != 0.0f && scale != 1.0f)
            {
                if (scale < 0.0f)
                {
                    // Negative scale = horizontal flip with magnitude
                    auto texSize = cl.texture->getSize();
                    sprite.setOrigin({static_cast<float>(texSize.x), 0.0f});
                    sprite.setScale({-std::abs(scale), std::abs(scale)});
                }
                else
                {
                    sprite.setScale({scale, scale});
                }
            }

            target.draw(sprite);
        }
    }

    if (!m_cellDebugLogged[layer])
    {
        m_cellDebugLogged[layer] = true;
        LOG_INFO("MapRenderer::renderCellLayer(%d): range=[%d..%d][%d..%d], cam=(%.1f,%.1f), found=%d visible=%d loaded=%d",
                 layer, startRow, endRow, startCol, endCol, camX, camY, cellTotal, cellVisible, cellLoaded);
    }
}
