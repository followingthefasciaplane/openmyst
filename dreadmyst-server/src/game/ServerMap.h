#pragma once
#include "core/Types.h"
#include "game/templates/MapTemplate.h"
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>

class ServerPlayer;
class ServerNpc;
class ServerGameObj;
class ServerUnit;
class GamePacket;

// Server-side map instance with its own thread
// Matches the original ServerMap.cpp - each map runs in its own thread
// with mutex+condition variable synchronization
class ServerMap {
public:
    explicit ServerMap(const MapTemplate& tmpl);
    ~ServerMap();

    // Map info
    uint32 getId() const { return m_template.id; }
    const std::string& getName() const { return m_template.name; }
    const MapTemplate& getTemplate() const { return m_template; }

    // Thread management
    void startThread();
    void stopThread();
    void signalTick();

    // Player management
    void addPlayer(std::shared_ptr<ServerPlayer> player);
    void removePlayer(Guid guid);
    ServerPlayer* findPlayer(Guid guid) const;
    std::vector<std::shared_ptr<ServerPlayer>> getPlayers() const;
    uint32 getPlayerCount() const;

    // NPC management
    void addNpc(std::shared_ptr<ServerNpc> npc);
    void removeNpc(Guid guid);
    ServerNpc* findNpc(Guid guid) const;
    std::vector<std::shared_ptr<ServerNpc>> getNpcs() const;

    // Game object management
    void addGameObj(std::shared_ptr<ServerGameObj> obj);
    void removeGameObj(Guid guid);
    ServerGameObj* findGameObj(Guid guid) const;

    // Broadcasting
    void broadcastPacket(const GamePacket& packet);
    void broadcastPacketExcept(const GamePacket& packet, Guid excludeGuid);
    void sendToPlayersInRange(const GamePacket& packet, float x, float y, float range);

    // Queries
    ServerNpc* findNearestNpc(float x, float y, float maxRange = 0.0f) const;
    std::vector<ServerUnit*> getUnitsInRange(float x, float y, float range) const;

    // Map update (called from map thread)
    void update(int32 deltaMs);

private:
    void threadMain();
    void updateNpcs(int32 deltaMs);
    void updatePlayers(int32 deltaMs);
    void updateRespawns(int32 deltaMs);

    MapTemplate m_template;

    // Thread state
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_threadRunning = false;
    bool m_tickReady     = false;

    // Entities on this map
    std::unordered_map<Guid, std::shared_ptr<ServerPlayer>> m_players;
    std::unordered_map<Guid, std::shared_ptr<ServerNpc>>    m_npcs;
    std::unordered_map<Guid, std::shared_ptr<ServerGameObj>> m_gameObjs;

    // Respawn tracking
    struct RespawnEntry {
        Guid   guid       = 0;
        uint32 entry      = 0;
        int32  timerMs    = 0;
        float  x          = 0.0f;
        float  y          = 0.0f;
        float  orientation = 0.0f;
        bool   isNpc      = true;
    };
    std::vector<RespawnEntry> m_respawnQueue;
};
