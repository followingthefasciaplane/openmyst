#pragma once
#include "game/MutualObject.h"
#include "game/templates/GameObjectTemplate.h"
#include "game/templates/ItemTemplate.h"
#include <vector>

class ServerPlayer;
class GameCache;

// Game object state
enum class GameObjState : uint32 {
    Ready     = 0,
    Active    = 1,
    Disabled  = 2,
    Despawned = 3
};

// Server-side game object matching ServerGameObj.cpp from the original binary
// Types: NullType (0), Container (1), MapChanger (2), Waypoint (3)
class ServerGameObj : public MutualObject {
public:
    ServerGameObj(const GameObjectTemplate& tmpl, const GameObjectSpawn& spawn);
    ~ServerGameObj() override = default;

    // -- Template info --
    uint32 getTemplateEntry() const { return m_templateEntry; }
    const GameObjectTemplate& getTemplate() const { return m_template; }
    GameObjType getType() const { return static_cast<GameObjType>(m_template.type); }

    // -- State --
    GameObjState getState() const { return m_state; }
    void setState(GameObjState state) { m_state = state; }
    bool isReady() const { return m_state == GameObjState::Ready; }

    // -- Respawn --
    int32  getRespawnTime() const { return m_respawnTimeMs; }
    bool   isRespawning() const { return m_state == GameObjState::Despawned; }
    void   setRespawnTime(int32 ms) { m_respawnTimeMs = ms; }

    // -- Requirements --
    uint32 getRequiredItem() const { return m_template.requiredItem; }
    uint32 getRequiredQuest() const { return m_template.requiredQuest; }

    // -- Interaction --
    bool isUsable(ServerPlayer* player) const;
    void interact(ServerPlayer* player);

    // -- Container (type 1) --
    const std::vector<ItemInstance>& getLoot() const { return m_loot; }
    void setLoot(const std::vector<ItemInstance>& loot) { m_loot = loot; }
    void clearLoot() { m_loot.clear(); }
    bool isLooted() const { return m_looted; }

    // -- MapChanger (type 2) --
    uint32 getTargetMapId() const;
    float  getTargetX() const;
    float  getTargetY() const;

    // -- Waypoint (type 3) --
    // Discovery tracked in player's discovered_waypoints set

    // -- Update --
    void update(int32 deltaMs);

    // -- Respawn logic --
    void despawn();
    void respawn();

    // -- GameCache reference (set by ServerMap on spawn) --
    void setGameCache(const GameCache* cache) { m_gameCache = cache; }
    const GameCache* getGameCache() const { return m_gameCache; }

private:
    void initFromTemplate(const GameObjectTemplate& tmpl, const GameObjectSpawn& spawn);

    void interactContainer(ServerPlayer* player);
    void interactMapChanger(ServerPlayer* player);
    void interactWaypoint(ServerPlayer* player);

    bool checkRequiredItem(ServerPlayer* player) const;
    bool checkRequiredQuest(ServerPlayer* player) const;

    // Template data (cached copy)
    GameObjectTemplate m_template;
    uint32             m_templateEntry = 0;

    // State
    GameObjState m_state = GameObjState::Ready;

    // Respawn
    int32 m_respawnTimeMs = 0;
    int32 m_respawnTimer  = 0;

    // Container loot
    std::vector<ItemInstance> m_loot;
    bool m_looted = false;

    // Cache reference
    const GameCache* m_gameCache = nullptr;
};
