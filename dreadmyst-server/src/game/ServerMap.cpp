#include "game/ServerMap.h"
#include "game/ServerPlayer.h"
#include "game/ServerNpc.h"
#include "game/ServerGameObj.h"
#include "game/GameCache.h"
#include "core/Server.h"
#include "network/GamePacket.h"
#include "core/Log.h"
#include <algorithm>
#include <cmath>

ServerMap::ServerMap(const MapTemplate& tmpl)
    : m_template(tmpl) {
}

ServerMap::~ServerMap() {
    stopThread();
}

void ServerMap::startThread() {
    m_threadRunning = true;
    m_thread = std::thread(&ServerMap::threadMain, this);
    LOG_INFO("Map '%s' (id=%u) thread started", m_template.name.c_str(), m_template.id);
}

void ServerMap::stopThread() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_threadRunning = false;
        m_tickReady = true;
    }
    m_cv.notify_one();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ServerMap::signalTick() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tickReady = true;
    }
    m_cv.notify_one();
}

void ServerMap::threadMain() {
    while (m_threadRunning) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return m_tickReady || !m_threadRunning; });

        if (!m_threadRunning) break;

        m_tickReady = false;
        lock.unlock();

        update(TICK_INTERVAL_MS);
    }
}

void ServerMap::addPlayer(std::shared_ptr<ServerPlayer> player) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_players[player->getGuid()] = player;
    player->setMap(this);
    LOG_INFO("Player '%s' (guid=%u) added to map '%s'",
             player->getName().c_str(), player->getGuid(), m_template.name.c_str());
}

void ServerMap::removePlayer(Guid guid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_players.find(guid);
    if (it != m_players.end()) {
        it->second->setMap(nullptr);
        m_players.erase(it);
    }
}

ServerPlayer* ServerMap::findPlayer(Guid guid) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    auto it = m_players.find(guid);
    return it != m_players.end() ? it->second.get() : nullptr;
}

std::vector<std::shared_ptr<ServerPlayer>> ServerMap::getPlayers() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    std::vector<std::shared_ptr<ServerPlayer>> result;
    result.reserve(m_players.size());
    for (auto& [guid, player] : m_players) {
        result.push_back(player);
    }
    return result;
}

uint32 ServerMap::getPlayerCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    return static_cast<uint32>(m_players.size());
}

void ServerMap::addNpc(std::shared_ptr<ServerNpc> npc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_npcs[npc->getGuid()] = npc;
    npc->setMap(this);
}

void ServerMap::removeNpc(Guid guid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_npcs.erase(guid);
}

ServerNpc* ServerMap::findNpc(Guid guid) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    auto it = m_npcs.find(guid);
    return it != m_npcs.end() ? it->second.get() : nullptr;
}

std::vector<std::shared_ptr<ServerNpc>> ServerMap::getNpcs() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    std::vector<std::shared_ptr<ServerNpc>> result;
    result.reserve(m_npcs.size());
    for (auto& [guid, npc] : m_npcs) {
        result.push_back(npc);
    }
    return result;
}

void ServerMap::addGameObj(std::shared_ptr<ServerGameObj> obj) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gameObjs[obj->getGuid()] = obj;
}

void ServerMap::removeGameObj(Guid guid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gameObjs.erase(guid);
}

ServerGameObj* ServerMap::findGameObj(Guid guid) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    auto it = m_gameObjs.find(guid);
    return it != m_gameObjs.end() ? it->second.get() : nullptr;
}

void ServerMap::broadcastPacket(const GamePacket& packet) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [guid, player] : m_players) {
        player->sendPacket(packet);
    }
}

void ServerMap::broadcastPacketExcept(const GamePacket& packet, Guid excludeGuid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [guid, player] : m_players) {
        if (guid != excludeGuid) {
            player->sendPacket(packet);
        }
    }
}

void ServerMap::sendToPlayersInRange(const GamePacket& packet, float x, float y, float range) {
    std::lock_guard<std::mutex> lock(m_mutex);
    float rangeSq = range * range;
    for (auto& [guid, player] : m_players) {
        float dx = player->getPositionX() - x;
        float dy = player->getPositionY() - y;
        if (dx * dx + dy * dy <= rangeSq) {
            player->sendPacket(packet);
        }
    }
}

ServerNpc* ServerMap::findNearestNpc(float x, float y, float maxRange) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    ServerNpc* nearest = nullptr;
    float nearestDistSq = maxRange > 0.0f ? maxRange * maxRange : 1e18f;

    for (auto& [guid, npc] : m_npcs) {
        float dx = npc->getPositionX() - x;
        float dy = npc->getPositionY() - y;
        float distSq = dx * dx + dy * dy;
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearest = npc.get();
        }
    }
    return nearest;
}

std::vector<ServerUnit*> ServerMap::getUnitsInRange(float x, float y, float range) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    std::vector<ServerUnit*> result;
    float rangeSq = range * range;

    for (auto& [guid, player] : m_players) {
        float dx = player->getPositionX() - x;
        float dy = player->getPositionY() - y;
        if (dx * dx + dy * dy <= rangeSq) {
            result.push_back(player.get());
        }
    }
    for (auto& [guid, npc] : m_npcs) {
        float dx = npc->getPositionX() - x;
        float dy = npc->getPositionY() - y;
        if (dx * dx + dy * dy <= rangeSq) {
            result.push_back(npc.get());
        }
    }
    return result;
}

void ServerMap::update(int32 deltaMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    updateNpcs(deltaMs);
    updatePlayers(deltaMs);
    updateRespawns(deltaMs);
}

void ServerMap::updateNpcs(int32 deltaMs) {
    for (auto& [guid, npc] : m_npcs) {
        npc->update(deltaMs);
    }
}

void ServerMap::updatePlayers(int32 deltaMs) {
    for (auto& [guid, player] : m_players) {
        player->update(deltaMs);
    }
}

void ServerMap::updateRespawns(int32 deltaMs) {
    GameCache* cache = Server::instance().getGameCache();
    if (!cache) return;

    for (auto it = m_respawnQueue.begin(); it != m_respawnQueue.end(); ) {
        it->timerMs -= deltaMs;
        if (it->timerMs <= 0) {
            if (it->isNpc) {
                const NpcTemplate* npcTmpl = cache->getNpcTemplate(it->entry);
                if (npcTmpl) {
                    NpcSpawn spawn;
                    spawn.guid = it->guid;
                    spawn.entry = it->entry;
                    spawn.map = m_template.id;
                    spawn.positionX = it->x;
                    spawn.positionY = it->y;
                    spawn.orientation = it->orientation;

                    auto npc = std::make_shared<ServerNpc>(*npcTmpl, spawn);
                    npc->setGameCache(cache);
                    // Add directly (already holding m_mutex in update())
                    m_npcs[npc->getGuid()] = npc;
                    npc->setMap(this);
                    LOG_DEBUG("Respawned NPC guid=%u entry=%u on map '%s'",
                              it->guid, it->entry, m_template.name.c_str());
                }
            } else {
                const GameObjectTemplate* goTmpl = cache->getGameObjTemplate(it->entry);
                if (goTmpl) {
                    GameObjectSpawn spawn;
                    spawn.guid = it->guid;
                    spawn.entry = it->entry;
                    spawn.map = m_template.id;
                    spawn.positionX = it->x;
                    spawn.positionY = it->y;

                    auto obj = std::make_shared<ServerGameObj>(*goTmpl, spawn);
                    obj->setGameCache(cache);
                    m_gameObjs[obj->getGuid()] = obj;
                    LOG_DEBUG("Respawned GameObject guid=%u entry=%u on map '%s'",
                              it->guid, it->entry, m_template.name.c_str());
                }
            }
            it = m_respawnQueue.erase(it);
        } else {
            ++it;
        }
    }
}
