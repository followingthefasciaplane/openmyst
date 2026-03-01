#include "MapCell.h"

void MapCellClient::setLayer(int layer, const std::string& texName, float scale)
{
    if (layer < 0 || layer >= 3) return;
    m_layers[layer].textureName = texName;
    m_layers[layer].scale = scale;
}

bool MapCellClient::hasAnyTexture() const
{
    for (int i = 0; i < 3; i++)
    {
        if (!m_layers[i].textureName.empty())
            return true;
    }
    return false;
}
