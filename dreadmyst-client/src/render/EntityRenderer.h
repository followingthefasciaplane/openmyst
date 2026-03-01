#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include "render/ModelScript.h"

class WorldState;
class Camera;

// EntityRenderer - draws all world entities using texture atlas sprites
// defined by model script files (scripts/npc/*.txt, scripts/player/*.txt).
//
// Binary: ModelScriptRender at 0x004ce830, loadModelScripts at 0x0043c810

class EntityRenderer
{
public:
    EntityRenderer();

    // Call once per frame BEFORE render() to advance animations.
    void update(float dt, uint32_t localPlayerGuid, bool localPlayerMoving,
                float localPlayerOrientation, int localPlayerGender);

    // Render all entities from WorldState + local player.
    void render(sf::RenderTarget& target, float cameraX, float cameraY,
                unsigned int screenWidth, unsigned int screenHeight,
                uint32_t localPlayerGuid, float localPlayerX, float localPlayerY,
                int localPlayerGender);

private:
    void gridToScreen(float col, float row, float camX, float camY,
                      float& outScreenX, float& outScreenY) const;

    static int orientationToDirection(float orientation);

    sf::Font m_font;
    bool m_fontLoaded = false;

    // Local player animation state
    ModelAnimator m_localPlayerAnimator;
    std::shared_ptr<ModelScript> m_localPlayerScript;
    bool m_localPlayerModelResolved = false;
};
