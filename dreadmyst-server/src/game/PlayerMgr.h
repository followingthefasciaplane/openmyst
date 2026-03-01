#pragma once
#include "core/Types.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class ServerPlayer;
class Session;

class PlayerMgr {
public:
    static PlayerMgr& instance();

    void addPlayer(std::shared_ptr<ServerPlayer> player);
    void removePlayer(Guid guid);
    ServerPlayer* findPlayer(Guid guid) const;
    ServerPlayer* findPlayerByName(const std::string& name) const;
    ServerPlayer* findPlayerByAccount(uint32 accountId) const;
    uint32 getPlayerCount() const;

    // Broadcast to all online players
    void broadcastMessage(const std::string& msg);
    void broadcastSystemMessage(const std::string& msg);

    // Save all players
    void saveAllPlayers();

    // Update all players (called each tick)
    void updateAll(int32 deltaMs);

private:
    PlayerMgr() = default;
    mutable std::mutex m_mutex;
    std::unordered_map<Guid, std::shared_ptr<ServerPlayer>> m_players;
};
