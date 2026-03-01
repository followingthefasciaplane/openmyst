#include "EntityRenderer.h"
#include "game/WorldState.h"
#include "network/PacketHandler.h"
#include "GameDefines.h"
#include "UnitDefines.h"
#include "render/SpriteManager.h"
#include "resource/ContentDB.h"
#include "core/Log.h"

EntityRenderer::EntityRenderer()
{
    static const char* fontPaths[] = {
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
    };

    for (const auto* path : fontPaths)
    {
        if (m_font.openFromFile(path))
        {
            m_fontLoaded = true;
            break;
        }
    }
}

int EntityRenderer::orientationToDirection(float orientation)
{
    int dir = static_cast<int>(orientation) % 8;
    if (dir < 0) dir += 8;
    return dir;
}

// Map UnitDefines::AnimId to ModelScript AnimName
static int animIdToScriptAnim(int unitAnimId, bool isMoving)
{
    // For movement, always use Run animation
    if (isMoving)
        return AnimName::Run;

    switch (unitAnimId)
    {
        case 0: return AnimName::Stance;  // Idle
        case 1: return AnimName::Run;     // Walk
        case 2: return AnimName::Run;     // Run
        case 3: return AnimName::Swing;   // Attack
        case 4: return AnimName::Swing;   // AttackOffhand
        case 5: return AnimName::Cast;    // CastStart
        case 6: return AnimName::Cast;    // CastChannel
        case 7: return AnimName::Die;     // Death
        case 8: return AnimName::Hit;     // Damaged
        case 9: return AnimName::Block;   // Block
        default: return AnimName::Stance;
    }
}

void EntityRenderer::update(float dt, uint32_t localPlayerGuid, bool localPlayerMoving,
                             float localPlayerOrientation, int localPlayerGender)
{
    // Update local player animator
    {
        if (!m_localPlayerModelResolved)
        {
            // Load the default_chest body layer for the local player
            m_localPlayerScript = sSpriteMgr.getPlayerSprite("default_chest", localPlayerGender);
            if (m_localPlayerScript)
            {
                m_localPlayerAnimator.setModelScript(m_localPlayerScript.get());
                m_localPlayerAnimator.setAnimation(AnimName::Stance);
            }
            m_localPlayerModelResolved = true;
        }

        int dir = orientationToDirection(localPlayerOrientation);
        m_localPlayerAnimator.setDirection(dir);

        int scriptAnim = localPlayerMoving ? AnimName::Run : AnimName::Stance;
        m_localPlayerAnimator.setAnimation(scriptAnim);
        m_localPlayerAnimator.update(dt);
    }

    // Update all world entity animators
    for (auto& [guid, ent] : sWorldState.getAllEntities())
    {
        // Resolve model on first encounter
        if (!ent.modelResolved)
        {
            ent.modelResolved = true;

            if (ent.type == ObjDefines::Type_Npc && ent.entryId > 0)
            {
                int modelId = 0;
                float scale = 1.0f;
                if (sContentDB.getNpcTemplateModelId(static_cast<int>(ent.entryId),
                                                      modelId, scale))
                {
                    NpcModelInfo modelInfo;
                    if (sContentDB.getNpcModel(modelId, modelInfo))
                    {
                        ent.modelName = modelInfo.name;
                        ent.modelScale = scale;

                        // Load NPC model script
                        auto script = sSpriteMgr.getNpcSprite(ent.modelName);
                        if (script)
                        {
                            ent.animator.setModelScript(script.get());
                            ent.animator.setAnimation(AnimName::Stance);
                        }
                    }
                }
            }
            else if (ent.type == ObjDefines::Type_Player)
            {
                // Load player body sprite
                auto script = sSpriteMgr.getPlayerSprite("default_chest", ent.gender);
                if (script)
                {
                    ent.animator.setModelScript(script.get());
                    ent.animator.setAnimation(AnimName::Stance);
                }
            }
        }

        // Update direction from orientation
        int dir = orientationToDirection(ent.orientation);
        ent.animator.setDirection(dir);

        // Select animation
        int scriptAnim = ent.isMoving ? AnimName::Run : AnimName::Stance;
        ent.animator.setAnimation(scriptAnim);
        ent.animator.update(dt);
    }
}

void EntityRenderer::gridToScreen(float col, float row, float camX, float camY,
                                   float& outScreenX, float& outScreenY) const
{
    outScreenX = (col * 32.f - row * 32.f) + camX;
    outScreenY = (row * 16.f + col * 16.f) + camY;
}

void EntityRenderer::render(sf::RenderTarget& target, float cameraX, float cameraY,
                             unsigned int screenWidth, unsigned int screenHeight,
                             uint32_t localPlayerGuid, float localPlayerX, float localPlayerY,
                             int localPlayerGender)
{
    const float entityRadius = 10.f;
    const float goSize = 16.f;
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);

    // Helper: draw a model-scripted sprite or fallback shape
    auto drawEntity = [&](float sx, float sy, const ModelAnimator& anim,
                          const ModelScript* script,
                          float scale, const std::string& name,
                          sf::Color fallbackColor, sf::Color nameColor)
    {
        bool spriteDrawn = false;

        if (script && script->isValid())
        {
            sf::IntRect frameRect;
            int offX = 0, offY = 0;
            if (anim.getCurrentFrame(frameRect, offX, offY))
            {
                sf::Sprite sprite(*script->getTexture(), frameRect);

                // Origin at the offset point (defines where the "foot" is)
                sprite.setOrigin({static_cast<float>(offX), static_cast<float>(offY)});

                if (scale != 1.0f)
                    sprite.setScale({scale, scale});

                sprite.setPosition({sx, sy});
                target.draw(sprite);
                spriteDrawn = true;
            }
        }

        if (!spriteDrawn)
        {
            // Fallback: colored shape
            sf::CircleShape circle(entityRadius);
            circle.setFillColor(fallbackColor);
            circle.setOutlineColor(sf::Color::White);
            circle.setOutlineThickness(1.f);
            circle.setOrigin({entityRadius, entityRadius});
            circle.setPosition({sx, sy});
            target.draw(circle);
        }

        // Draw name label
        if (m_fontLoaded && !name.empty())
        {
            sf::Text nameText(m_font, name, 11);
            nameText.setFillColor(nameColor);
            nameText.setOutlineColor(sf::Color::Black);
            nameText.setOutlineThickness(1.f);
            auto bounds = nameText.getLocalBounds();
            nameText.setOrigin({bounds.size.x / 2.f, bounds.size.y});

            float nameOffsetY = entityRadius + 4.f;
            if (spriteDrawn)
            {
                // Get current frame height for name positioning
                sf::IntRect r;
                int ox, oy;
                if (anim.getCurrentFrame(r, ox, oy))
                    nameOffsetY = static_cast<float>(oy) * scale + 4.f;
            }

            nameText.setPosition({sx, sy - nameOffsetY});
            target.draw(nameText);
        }
    };

    // Draw local player
    {
        float sx, sy;
        gridToScreen(localPlayerX, localPlayerY, cameraX, cameraY, sx, sy);

        if (sx > -100.f && sx < sw + 100.f && sy > -100.f && sy < sh + 100.f)
        {
            const auto& player = sPacketHandler.getLocalPlayer();
            drawEntity(sx, sy, m_localPlayerAnimator,
                       m_localPlayerScript.get(),
                       1.0f, player.name,
                       sf::Color(50, 200, 50, 200), sf::Color::White);
        }
    }

    // Draw all world entities
    for (const auto& [guid, ent] : sWorldState.getAllEntities())
    {
        float sx, sy;
        gridToScreen(ent.posX, ent.posY, cameraX, cameraY, sx, sy);

        if (sx < -100.f || sx > sw + 100.f || sy < -100.f || sy > sh + 100.f)
            continue;

        switch (ent.type)
        {
            case ObjDefines::Type_Player:
            {
                // Get the script for this player's gender
                auto script = sSpriteMgr.getPlayerSprite("default_chest", ent.gender);
                drawEntity(sx, sy, ent.animator, script.get(),
                           1.0f, ent.name,
                           sf::Color(80, 120, 255, 200),
                           sf::Color(100, 180, 255));
                break;
            }

            case ObjDefines::Type_Npc:
            {
                std::shared_ptr<ModelScript> script;
                if (!ent.modelName.empty())
                    script = sSpriteMgr.getNpcSprite(ent.modelName);
                drawEntity(sx, sy, ent.animator, script.get(),
                           ent.modelScale, ent.name,
                           sf::Color(220, 60, 60, 200),
                           sf::Color(255, 100, 100));
                break;
            }

            case ObjDefines::Type_GameObject:
            {
                sf::RectangleShape rect({goSize, goSize});
                rect.setFillColor(sf::Color(230, 200, 50, 180));
                rect.setOutlineColor(sf::Color::White);
                rect.setOutlineThickness(1.f);
                rect.setOrigin({goSize / 2.f, goSize / 2.f});
                rect.setPosition({sx, sy});
                target.draw(rect);
                break;
            }
            default:
                break;
        }
    }
}
