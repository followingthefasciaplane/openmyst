#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

// SpriteSheet - wraps a texture with frame grid metadata.
//
// Layout: rows = directions (8 for full isometric, fewer for simple NPCs),
// columns = animation frames. Frame size = npc_models.height (square frames).
// Extra pixels at right/bottom edges of the sheet are unused padding.

class SpriteSheet
{
public:
    SpriteSheet() = default;

    void init(std::shared_ptr<sf::Texture> texture, int frameWidth, int frameHeight);

    // Get texture rect for a specific direction row + frame column.
    sf::IntRect getFrameRect(int direction, int frameIndex) const;

    int getFrameWidth()    const { return m_frameWidth; }
    int getFrameHeight()   const { return m_frameHeight; }
    int getFramesPerRow()  const { return m_framesPerRow; }
    int getNumRows()       const { return m_numRows; }

    sf::Texture* getTexture() const { return m_texture.get(); }
    bool isValid() const { return m_texture != nullptr && m_frameWidth > 0; }

private:
    std::shared_ptr<sf::Texture> m_texture;
    int m_frameWidth   = 0;
    int m_frameHeight  = 0;
    int m_framesPerRow = 0;   // columns in sheet
    int m_numRows      = 0;   // total direction rows
};

inline void SpriteSheet::init(std::shared_ptr<sf::Texture> texture, int frameWidth, int frameHeight)
{
    m_texture     = std::move(texture);
    m_frameWidth  = frameWidth;
    m_frameHeight = frameHeight;

    if (m_texture && m_frameWidth > 0 && m_frameHeight > 0)
    {
        auto sz = m_texture->getSize();
        m_framesPerRow = static_cast<int>(sz.x) / m_frameWidth;
        m_numRows      = static_cast<int>(sz.y) / m_frameHeight;
    }
}

inline sf::IntRect SpriteSheet::getFrameRect(int direction, int frameIndex) const
{
    int col = (m_framesPerRow > 0) ? (frameIndex % m_framesPerRow) : 0;
    int row = direction;
    if (row >= m_numRows) row = 0;

    return sf::IntRect(
        {col * m_frameWidth, row * m_frameHeight},
        {m_frameWidth, m_frameHeight}
    );
}
