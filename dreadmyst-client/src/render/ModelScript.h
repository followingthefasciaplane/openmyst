#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <SFML/Graphics.hpp>

// ModelScript - parsed texture atlas animation data from model script files.
//
// Binary source: ModelScript.cpp (0x004cc810 parse, 0x004ce830 render)
//
// Each model script defines animations (stance, run, cast, etc.) for a sprite.
// Frames are NOT a regular grid - each frame has explicit pixel coordinates,
// dimensions, and render offsets within the texture atlas PNG.
//
// Format:
//   image=filename.png
//   [animation_name]
//   frames=N
//   duration=Nms
//   type=looped|play_once|back_forth
//   frame=FRAME_IDX,DIRECTION,X,Y,WIDTH,HEIGHT,OFFSET_X,OFFSET_Y

enum class AnimPlayType
{
    Looped    = 0,
    PlayOnce  = 1,
    BackForth = 2,
};

struct FrameData
{
    int x, y;           // top-left in atlas
    int width, height;  // frame dimensions (vary per frame)
    int offsetX, offsetY; // render offset for alignment
};

struct AnimationDef
{
    std::string name;
    int frameCount = 0;
    float durationSec = 0.f;
    AnimPlayType playType = AnimPlayType::Looped;

    // frames[frameIndex][direction] = FrameData
    // Direction 0-7: S, SW, W, NW, N, NE, E, SE
    std::vector<std::vector<FrameData>> frames; // [frameCount][8]
};

// Maps animation name -> animation ID used by the engine
// These map to UnitDefines::AnimId values
namespace AnimName
{
    constexpr int Stance = 0;   // idle
    constexpr int Run    = 1;   // walk/run
    constexpr int Shoot  = 2;
    constexpr int Cast   = 3;
    constexpr int Swing  = 4;
    constexpr int Hit    = 5;
    constexpr int Die    = 6;
    constexpr int Block  = 7;
    constexpr int CritDie = 8;
    constexpr int Max    = 9;
}

class ModelScript
{
public:
    bool loadFromFile(const std::string& path, const std::string& imagePrefix = "");
    bool isValid() const { return m_texture != nullptr && !m_animations.empty(); }

    const std::string& getImageName() const { return m_imageName; }
    std::shared_ptr<sf::Texture> getTexture() const { return m_texture; }

    // Get animation by engine ID (AnimName::Stance, etc.)
    const AnimationDef* getAnimation(int animId) const;

    // Get animation count
    int getAnimationCount() const { return static_cast<int>(m_animations.size()); }

private:
    static int nameToAnimId(const std::string& name);

    std::string m_imageName;
    std::shared_ptr<sf::Texture> m_texture;
    std::unordered_map<int, AnimationDef> m_animations; // keyed by AnimName::*
};

// ModelAnimator - per-entity animation state using ModelScript data.
// Replaces SpriteAnimator for atlas-based sprites.
class ModelAnimator
{
public:
    void setModelScript(const ModelScript* script);
    void setAnimation(int animId);
    void setDirection(int dir);
    void update(float dt);

    // Get current frame's texture rect and render offset.
    // Returns false if no valid frame.
    bool getCurrentFrame(sf::IntRect& outRect, int& outOffsetX, int& outOffsetY) const;

    int getAnimId()   const { return m_animId; }
    int getDirection() const { return m_direction; }
    bool isFinished()  const { return m_finished; }
    void reset();

private:
    const ModelScript* m_script = nullptr;
    const AnimationDef* m_currentAnim = nullptr;
    int m_animId     = -1;
    int m_direction  = 0;
    int m_frame      = 0;
    float m_timer    = 0.f;
    bool m_finished  = false;
    bool m_reverse   = false; // for back_forth
};
