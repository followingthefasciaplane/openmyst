#include "game/ServerGameObj.h"
#include "game/ServerPlayer.h"
#include "game/GameCache.h"
#include "core/Log.h"

// Data field indices for MapChanger template (data[0..2])
static constexpr int DATA_TARGET_MAP = 0;
static constexpr int DATA_TARGET_X   = 1;
static constexpr int DATA_TARGET_Y   = 2;

// ============================================================================
// ServerGameObj
// ============================================================================

ServerGameObj::ServerGameObj(const GameObjectTemplate& tmpl, const GameObjectSpawn& spawn) {
    initFromTemplate(tmpl, spawn);
}

void ServerGameObj::initFromTemplate(const GameObjectTemplate& tmpl, const GameObjectSpawn& spawn) {
    m_template      = tmpl;
    m_templateEntry = tmpl.entry;

    // MutualObject fields from spawn data
    setGuid(spawn.guid);
    setEntry(spawn.entry);
    setMapId(spawn.map);
    setPosition(spawn.positionX, spawn.positionY);

    // State from spawn data
    m_state = static_cast<GameObjState>(spawn.state);

    // Respawn time (seconds -> milliseconds)
    m_respawnTimeMs = static_cast<int32>(spawn.respawn) * 1000;
    m_respawnTimer  = 0;

    LOG_DEBUG("ServerGameObj::init entry=%u guid=%u type=%u mapId=%u",
              tmpl.entry, spawn.guid, tmpl.type, spawn.map);
}

// -- Requirements --

bool ServerGameObj::checkRequiredItem(ServerPlayer* player) const {
    if (m_template.requiredItem == 0) return true;
    return player->hasItem(m_template.requiredItem);
}

bool ServerGameObj::checkRequiredQuest(ServerPlayer* player) const {
    if (m_template.requiredQuest == 0) return true;
    return player->hasCompletedQuest(m_template.requiredQuest);
}

// -- Interaction --

bool ServerGameObj::isUsable(ServerPlayer* player) const {
    if (!player) return false;
    if (m_state != GameObjState::Ready) return false;

    // Check prerequisites
    if (!checkRequiredItem(const_cast<ServerPlayer*>(player))) return false;
    if (!checkRequiredQuest(const_cast<ServerPlayer*>(player))) return false;

    return true;
}

void ServerGameObj::interact(ServerPlayer* player) {
    if (!player) return;

    if (!isUsable(player)) {
        LOG_DEBUG("ServerGameObj::interact guid=%u not usable by player=%u",
                  getGuid(), player->getGuid());
        return;
    }

    GameObjType type = getType();
    switch (type) {
        case GameObjType::Container:
            interactContainer(player);
            break;
        case GameObjType::MapChanger:
            interactMapChanger(player);
            break;
        case GameObjType::Waypoint:
            interactWaypoint(player);
            break;
        case GameObjType::NullType:
        default:
            LOG_DEBUG("ServerGameObj::interact guid=%u type=%u has no interaction",
                      getGuid(), static_cast<uint32>(type));
            break;
    }
}

// -- Container --

void ServerGameObj::interactContainer(ServerPlayer* player) {
    if (m_looted) {
        LOG_DEBUG("ServerGameObj::interactContainer guid=%u already looted", getGuid());
        return;
    }

    // Generate loot if not yet populated
    if (m_loot.empty() && m_gameCache) {
        // Use template entry as loot ID to look up loot table
        const auto& lootEntries = m_gameCache->getLootEntries(m_templateEntry);
        for (const auto& entry : lootEntries) {
            float roll = static_cast<float>(std::rand() % 10000) / 100.0f;
            if (roll < entry.chance) {
                uint32 count = entry.countMin;
                if (entry.countMax > entry.countMin) {
                    count += std::rand() % (entry.countMax - entry.countMin + 1);
                }
                const ItemTemplate* itemTmpl = m_gameCache->getItemTemplate(entry.item);
                if (itemTmpl) {
                    ItemInstance inst;
                    inst.entry = itemTmpl->entry;
                    inst.count = count;
                    m_loot.push_back(inst);
                }
            }
        }
    }

    // Give loot items to the player
    for (const auto& item : m_loot) {
        player->addItemToInventory(item);
    }

    m_looted = true;

    LOG_INFO("ServerGameObj::interactContainer guid=%u player=%u items=%zu",
             getGuid(), player->getGuid(), m_loot.size());

    // Start despawn timer if configured
    if (m_respawnTimeMs > 0) {
        despawn();
    }
}

// -- MapChanger --

void ServerGameObj::interactMapChanger(ServerPlayer* player) {
    uint32 targetMap = getTargetMapId();
    float  targetX   = getTargetX();
    float  targetY   = getTargetY();

    LOG_INFO("ServerGameObj::interactMapChanger guid=%u player=%u -> map=%u pos=(%.1f, %.1f)",
             getGuid(), player->getGuid(), targetMap, targetX, targetY);

    // Teleport the player to the target map and position
    player->setPosition(targetX, targetY);
    player->setMapId(targetMap);
}

// -- Waypoint --

void ServerGameObj::interactWaypoint(ServerPlayer* player) {
    if (player->hasDiscoveredWaypoint(getGuid())) {
        LOG_DEBUG("ServerGameObj::interactWaypoint guid=%u already discovered by player=%u",
                  getGuid(), player->getGuid());
        return;
    }

    player->discoverWaypoint(getGuid());

    LOG_INFO("ServerGameObj::interactWaypoint guid=%u discovered by player=%u",
             getGuid(), player->getGuid());
}

// -- MapChanger data accessors --

uint32 ServerGameObj::getTargetMapId() const {
    return static_cast<uint32>(m_template.data[DATA_TARGET_MAP]);
}

float ServerGameObj::getTargetX() const {
    return static_cast<float>(m_template.data[DATA_TARGET_X]);
}

float ServerGameObj::getTargetY() const {
    return static_cast<float>(m_template.data[DATA_TARGET_Y]);
}

// -- Update --

void ServerGameObj::update(int32 deltaMs) {
    // Handle respawn countdown when despawned
    if (m_state == GameObjState::Despawned && m_respawnTimeMs > 0) {
        m_respawnTimer -= deltaMs;
        if (m_respawnTimer <= 0) {
            respawn();
        }
    }
}

// -- Despawn / Respawn --

void ServerGameObj::despawn() {
    m_state        = GameObjState::Despawned;
    m_respawnTimer = m_respawnTimeMs;
    m_loot.clear();

    LOG_DEBUG("ServerGameObj::despawn guid=%u entry=%u respawnMs=%d",
              getGuid(), m_templateEntry, m_respawnTimeMs);
}

void ServerGameObj::respawn() {
    m_state        = GameObjState::Ready;
    m_respawnTimer = 0;
    m_looted       = false;

    LOG_INFO("ServerGameObj::respawn guid=%u entry=%u", getGuid(), m_templateEntry);
}
