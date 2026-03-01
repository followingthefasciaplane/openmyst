#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

class ClientMap;

// MapRenderer - isometric tile renderer.
//
// Decompiled from GameMap::renderThread at 0x004ad950 and related functions.
//
// Determines visible cells from camera position, loads textures for
// visible cells via ResourceManager, draws layers back-to-front.
//
// Draw order (from binary render loop):
//   1. Terrain layer (13x13 cell ground tiles)
//   2. Cell Layer 0 (ground details)
//   3. Cell Layer 1 (lower objects)
//   4. Cell Layer 2 (upper objects / characters)
//
// Visibility determination (from binary):
//   - Convert screen center to starting cell via inverse isometric
//   - Expand outward from starting cell by (maxScreenDim / 64) + 14 cells
//   - For each cell, check if it's within screen bounds (frustum check)
//
// Binary constants:
//   Tile size: 64x32 pixels (isometric diamond)
//   Terrain tile: 832x416 pixels (13x cell size)
//   Default render margin: 14 cells beyond visible area

class MapRenderer
{
public:
    // Render the map to the given target.
    // Camera position is read from the ClientMap.
    void render(sf::RenderTarget& target, ClientMap& map,
                unsigned int screenWidth, unsigned int screenHeight);

private:
    // Render terrain tiles (large background ground tiles)
    void renderTerrain(sf::RenderTarget& target, ClientMap& map,
                       unsigned int screenWidth, unsigned int screenHeight);

    // Render cell layers
    void renderCellLayer(sf::RenderTarget& target, ClientMap& map,
                         int layer, unsigned int screenWidth, unsigned int screenHeight);

    // Debug flags (one-shot logging)
    bool m_debugLogged = false;
    bool m_terrainDebugLogged = false;
    bool m_cellDebugLogged[3] = {false, false, false};
};
