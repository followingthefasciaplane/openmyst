#include "game/PlayerMgr.h"
#include "game/ServerPlayer.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"
#include "core/Log.h"

// ============================================================================
// PlayerMgr - Singleton player session manager
// ============================================================================

PlayerMgr& PlayerMgr::instance() {
    static PlayerMgr inst;
    return inst;
}

void PlayerMgr::addPlayer(std::shared_ptr<ServerPlayer> player) {
    if (!player) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    Guid guid = player->getGuid();
    m_players[guid] = std::move(player);

    LOG_INFO("PlayerMgr::addPlayer guid=%u online=%u", guid, static_cast<uint32>(m_players.size()));
}

void PlayerMgr::removePlayer(Guid guid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(guid);
    if (it != m_players.end()) {
        m_players.erase(it);
        LOG_INFO("PlayerMgr::removePlayer guid=%u online=%u", guid, static_cast<uint32>(m_players.size()));
    }
}

ServerPlayer* PlayerMgr::findPlayer(Guid guid) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(guid);
    if (it != m_players.end()) {
        return it->second.get();
    }
    return nullptr;
}

ServerPlayer* PlayerMgr::findPlayerByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [guid, player] : m_players) {
        if (player->getName() == name) {
            return player.get();
        }
    }
    return nullptr;
}

ServerPlayer* PlayerMgr::findPlayerByAccount(uint32 accountId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [guid, player] : m_players) {
        if (player->getAccountId() == accountId) {
            return player.get();
        }
    }
    return nullptr;
}

uint32 PlayerMgr::getPlayerCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32>(m_players.size());
}

void PlayerMgr::broadcastMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);

    GamePacket packet(SMSG_CHAT_MSG);
    packet.writeString(msg);

    for (const auto& [guid, player] : m_players) {
        player->sendPacket(packet);
    }
}

void PlayerMgr::broadcastSystemMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);

    GamePacket packet(SMSG_CHAT_MSG);
    packet.writeUint32(0);  // sender guid 0 = system
    packet.writeString(msg);

    for (const auto& [guid, player] : m_players) {
        player->sendPacket(packet);
    }
}

void PlayerMgr::saveAllPlayers() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [guid, player] : m_players) {
        player->saveToDB();
    }

    LOG_INFO("PlayerMgr::saveAllPlayers saved %u players", static_cast<uint32>(m_players.size()));
}

void PlayerMgr::updateAll(int32 deltaMs) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [guid, player] : m_players) {
        player->update(deltaMs);
    }
}
