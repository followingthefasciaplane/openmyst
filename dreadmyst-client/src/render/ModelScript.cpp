#include "ModelScript.h"
#include "resource/ResourceManager.h"
#include "core/Log.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// Binary: ModelScript::parse at 0x004cc810
// Reads model script text files defining texture atlas animations.

int ModelScript::nameToAnimId(const std::string& name)
{
    if (name == "stance")  return AnimName::Stance;
    if (name == "run")     return AnimName::Run;
    if (name == "shoot")   return AnimName::Shoot;
    if (name == "cast")    return AnimName::Cast;
    if (name == "swing")   return AnimName::Swing;
    if (name == "hit")     return AnimName::Hit;
    if (name == "die")     return AnimName::Die;
    if (name == "block")   return AnimName::Block;
    if (name == "critdie") return AnimName::CritDie;
    return -1;
}

bool ModelScript::loadFromFile(const std::string& path, const std::string& imagePrefix)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    AnimationDef* currentAnim = nullptr;
    int currentAnimId = -1;

    while (std::getline(file, line))
    {
        // Trim whitespace and \r
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        if (line.empty())
            continue;

        // image= line
        if (line.rfind("image=", 0) == 0)
        {
            m_imageName = imagePrefix + line.substr(6);
            m_texture = sResourceMgr.getTexture(m_imageName);
            continue;
        }

        // [animation_name] section header
        if (line.front() == '[' && line.back() == ']')
        {
            std::string animName = line.substr(1, line.size() - 2);
            currentAnimId = nameToAnimId(animName);
            if (currentAnimId >= 0)
            {
                m_animations[currentAnimId] = AnimationDef();
                currentAnim = &m_animations[currentAnimId];
                currentAnim->name = animName;
            }
            else
            {
                currentAnim = nullptr;
            }
            continue;
        }

        if (!currentAnim)
            continue;

        // frames=N
        if (line.rfind("frames=", 0) == 0)
        {
            currentAnim->frameCount = std::stoi(line.substr(7));
            currentAnim->frames.resize(currentAnim->frameCount, std::vector<FrameData>(8));
            continue;
        }

        // duration=Nms
        if (line.rfind("duration=", 0) == 0)
        {
            std::string durStr = line.substr(9);
            // Remove "ms" suffix
            if (durStr.size() > 2 && durStr.substr(durStr.size() - 2) == "ms")
                durStr = durStr.substr(0, durStr.size() - 2);
            currentAnim->durationSec = std::stof(durStr) / 1000.f;
            continue;
        }

        // type=looped|play_once|back_forth
        if (line.rfind("type=", 0) == 0)
        {
            std::string typeStr = line.substr(5);
            if (typeStr == "looped")
                currentAnim->playType = AnimPlayType::Looped;
            else if (typeStr == "play_once")
                currentAnim->playType = AnimPlayType::PlayOnce;
            else if (typeStr == "back_forth")
                currentAnim->playType = AnimPlayType::BackForth;
            continue;
        }

        // frame=FRAME,DIR,X,Y,W,H,OFFX,OFFY
        if (line.rfind("frame=", 0) == 0)
        {
            std::string data = line.substr(6);
            std::replace(data.begin(), data.end(), ',', ' ');
            std::istringstream ss(data);

            int frameIdx, dir, x, y, w, h, ox, oy;
            if (ss >> frameIdx >> dir >> x >> y >> w >> h >> ox >> oy)
            {
                if (frameIdx >= 0 && frameIdx < currentAnim->frameCount &&
                    dir >= 0 && dir < 8)
                {
                    FrameData& fd = currentAnim->frames[frameIdx][dir];
                    fd.x = x;
                    fd.y = y;
                    fd.width = w;
                    fd.height = h;
                    fd.offsetX = ox;
                    fd.offsetY = oy;
                }
            }
            continue;
        }
    }

    if (!m_texture || m_animations.empty())
    {
        LOG_WARN("ModelScript: failed to load '%s' (texture=%s, anims=%d)",
                 path.c_str(), m_imageName.c_str(), (int)m_animations.size());
        return false;
    }

    return true;
}

const AnimationDef* ModelScript::getAnimation(int animId) const
{
    auto it = m_animations.find(animId);
    if (it != m_animations.end())
        return &it->second;
    return nullptr;
}

// ============================================================================
//  ModelAnimator - per-entity animation state
// ============================================================================

// Binary: ModelScriptRender::render at 0x004ce830

void ModelAnimator::setModelScript(const ModelScript* script)
{
    m_script = script;
    m_currentAnim = nullptr;
    m_animId = -1;
    m_frame = 0;
    m_timer = 0.f;
    m_finished = false;
    m_reverse = false;
}

void ModelAnimator::setAnimation(int animId)
{
    if (animId == m_animId)
        return;

    m_animId = animId;
    m_frame = 0;
    m_timer = 0.f;
    m_finished = false;
    m_reverse = false;

    if (m_script)
        m_currentAnim = m_script->getAnimation(animId);
    else
        m_currentAnim = nullptr;
}

void ModelAnimator::setDirection(int dir)
{
    if (dir >= 0 && dir < 8)
        m_direction = dir;
}

void ModelAnimator::update(float dt)
{
    if (!m_currentAnim || m_currentAnim->frameCount <= 1 || m_finished)
        return;

    float frameDur = m_currentAnim->durationSec / static_cast<float>(m_currentAnim->frameCount);
    if (frameDur <= 0.f)
        return;

    m_timer += dt;

    while (m_timer >= frameDur)
    {
        m_timer -= frameDur;

        switch (m_currentAnim->playType)
        {
            case AnimPlayType::Looped:
                m_frame = (m_frame + 1) % m_currentAnim->frameCount;
                break;

            case AnimPlayType::PlayOnce:
                if (m_frame < m_currentAnim->frameCount - 1)
                    ++m_frame;
                else
                    m_finished = true;
                break;

            case AnimPlayType::BackForth:
                if (!m_reverse)
                {
                    if (m_frame < m_currentAnim->frameCount - 1)
                        ++m_frame;
                    else
                        m_reverse = true;
                }
                else
                {
                    if (m_frame > 0)
                        --m_frame;
                    else
                        m_reverse = false;
                }
                break;
        }
    }
}

bool ModelAnimator::getCurrentFrame(sf::IntRect& outRect, int& outOffsetX, int& outOffsetY) const
{
    if (!m_currentAnim || m_currentAnim->frames.empty())
        return false;

    int frameIdx = m_frame;
    if (frameIdx < 0 || frameIdx >= static_cast<int>(m_currentAnim->frames.size()))
        frameIdx = 0;

    int dir = m_direction;
    if (dir < 0 || dir >= 8)
        dir = 0;

    const FrameData& fd = m_currentAnim->frames[frameIdx][dir];
    outRect = sf::IntRect({fd.x, fd.y}, {fd.width, fd.height});
    outOffsetX = fd.offsetX;
    outOffsetY = fd.offsetY;
    return true;
}

void ModelAnimator::reset()
{
    m_frame = 0;
    m_timer = 0.f;
    m_finished = false;
    m_reverse = false;
}
