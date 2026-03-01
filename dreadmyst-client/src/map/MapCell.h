#pragma once

#include "MapCellT.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

// MapCellClient - client-side map cell with texture data per layer.
//
// Decompiled from MapCellClient in binary:
//   0x004b3740 - loadTextures (lazy texture load on first access)
//   0x004b43c0 - getTextureName
//   Source: C:\...\MapCellClient.cpp (RTTI at 0x00639cd8)
//
// Each cell has up to 3 layers (ground, lower objects, upper objects).
// Layer 4 (overhead/ceiling) is terrain-level, not per-cell.
// Each layer stores a texture name (index into the map's texture list)
// and a scale factor (negative = flipped).

struct CellLayer
{
    std::string textureName;
    float scale = 1.0f;
    std::shared_ptr<sf::Texture> texture;
    bool loaded = false;
};

class MapCellClient : public MapCellT
{
public:
    // Set layer texture info (called during map loading)
    void setLayer(int layer, const std::string& texName, float scale);

    // Get the layer data
    const CellLayer& getLayer(int layer) const { return m_layers[layer]; }
    CellLayer& getLayer(int layer) { return m_layers[layer]; }

    bool hasAnyTexture() const;

private:
    CellLayer m_layers[3]; // Ground, Lower objects, Upper objects
};
