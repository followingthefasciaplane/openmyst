#include "core/Server.h"
#include "database/DatabaseMgr.h"
#include "network/NetworkMgr.h"
#include "game/GameCache.h"
#include "game/PlayerMgr.h"
#include "game/Guild.h"
#include "game/Arena.h"
#include "game/ServerMap.h"
#include "game/ServerNpc.h"
#include "game/ServerGameObj.h"

#include <SFML/Window/Keyboard.hpp>
#include <ctime>
#include <chrono>
#include <thread>
#include <set>

// Global accessor used by Gossip, Script, LootRoller, etc.
GameCache& getGameCache() {
    return *Server::instance().getGameCache();
}

// Reimplementation of the main server loop originally at 0x0040d260
// Server init at FUN_0045e130, tick at FUN_0045e440
// Shutdown flag was at 0x005c3b00 in the original binary
// Timing uses clock() for tick measurement, _time64 for unix timestamps

Server::Server() = default;
Server::~Server() = default;

Server& Server::instance() {
    static Server inst;
    return inst;
}

bool Server::initialize() {
    LOG_INFO("Server initializing (revision %d)...", DREADMYST_REVISION);

    auto& cfg = Config::instance();
    uint16 port = static_cast<uint16>(cfg.getInt("Tcp", "Port", DEFAULT_TCP_PORT));

    // Load all game templates from game.db (SQLite)
    // Database must already be initialized by caller (main.cpp calls DatabaseMgr::begin())
    m_gameCache = std::make_unique<GameCache>();
    if (!m_gameCache->loadAll(DatabaseMgr::instance().getGameDbConnection())) {
        LOG_ERROR("Failed to load game cache");
        return false;
    }
    LOG_INFO("Game cache loaded");

    // Initialize network listener
    m_networkMgr = std::make_unique<NetworkMgr>();
    if (!m_networkMgr->initialize(port)) {
        LOG_ERROR("Failed to bind to port %d", port);
        return false;
    }
    LOG_INFO("Network listening on port %d", port);

    // Create map instances from loaded templates
    // Collect all unique map IDs from NPC and game object spawns
    std::set<uint32> mapIds;
    for (const auto& spawn : m_gameCache->getNpcSpawns()) {
        mapIds.insert(spawn.map);
    }
    for (const auto& spawn : m_gameCache->getGameObjSpawns()) {
        mapIds.insert(spawn.map);
    }
    for (uint32 mapId : mapIds) {
        const MapTemplate* tmpl = m_gameCache->getMapTemplate(mapId);
        if (tmpl) {
            m_maps[mapId] = std::make_unique<ServerMap>(*tmpl);
            LOG_DEBUG("Created map instance %u ('%s')", mapId, tmpl->name.c_str());
        } else {
            MapTemplate fallback{};
            fallback.id = mapId;
            fallback.name = "Map_" + std::to_string(mapId);
            m_maps[mapId] = std::make_unique<ServerMap>(fallback);
            LOG_DEBUG("Created map instance %u (no template)", mapId);
        }
    }
    LOG_INFO("Map instances created (%zu maps)", m_maps.size());

    // Spawn NPCs on their respective maps
    uint32 npcCount = 0;
    for (const auto& spawn : m_gameCache->getNpcSpawns()) {
        auto mapIt = m_maps.find(spawn.map);
        if (mapIt == m_maps.end()) continue;

        const NpcTemplate* npcTmpl = m_gameCache->getNpcTemplate(spawn.entry);
        if (!npcTmpl) {
            LOG_WARN("NPC spawn entry %u has no template", spawn.entry);
            continue;
        }

        auto npc = std::make_shared<ServerNpc>(*npcTmpl, spawn);
        npc->setGameCache(m_gameCache.get());
        mapIt->second->addNpc(npc);
        npcCount++;
    }
    LOG_INFO("Spawned %u NPCs across %zu maps", npcCount, m_maps.size());

    // Spawn game objects on their respective maps
    uint32 goCount = 0;
    for (const auto& spawn : m_gameCache->getGameObjSpawns()) {
        auto mapIt = m_maps.find(spawn.map);
        if (mapIt == m_maps.end()) continue;

        const GameObjectTemplate* goTmpl = m_gameCache->getGameObjTemplate(spawn.entry);
        if (!goTmpl) {
            LOG_WARN("GameObj spawn entry %u has no template", spawn.entry);
            continue;
        }

        auto obj = std::make_shared<ServerGameObj>(*goTmpl, spawn);
        obj->setGameCache(m_gameCache.get());
        mapIt->second->addGameObj(obj);
        goCount++;
    }
    LOG_INFO("Spawned %u game objects across %zu maps", goCount, m_maps.size());

    return true;
}

void Server::run() {
    m_running.store(true);
    m_tickClock = clock();
    m_minuteAccum = 0;
    m_tenMinuteAccum = 0;

    LOG_INFO("Server main loop started (%d ms tick)", TICK_INTERVAL_MS);

    // Main loop structure matches binary at 0x00447560:
    //   while (!DAT_005c3b00) {
    //     loopStart = clock();
    //     serverTick(delta);
    //     do { mapTick(); sleep(); } while (clock()-loopStart < 100);
    //     if (CTRL+E) shutdown;
    //   }
    while (m_running.load()) {
        clock_t loopStart = clock();

        // --- Server tick (FUN_0045e440) ---
        clock_t now = clock();
        m_deltaTime = static_cast<uint32>(
            (now - m_tickClock) * 1000 / CLOCKS_PER_SEC);
        m_tickClock = now;
        m_msTime += m_deltaTime;
        m_unixTime = static_cast<int64>(std::time(nullptr));

        // Process network I/O first
        processPlayerConnections();

        // Shutdown countdown broadcast (binary checks this in FUN_0045e440)
        if (m_shutdownScheduled) {
            updateShutdownCountdown();
        }

        // Core per-tick processing
        processTimedEvents();

        // Periodic tasks (60s and 600s intervals from binary)
        m_minuteAccum += m_deltaTime;
        if (m_minuteAccum >= static_cast<uint32>(MINUTE_TICK_MS)) {
            m_minuteAccum -= MINUTE_TICK_MS;
            minuteTick();
        }

        m_tenMinuteAccum += m_deltaTime;
        if (m_tenMinuteAccum >= static_cast<uint32>(TEN_MINUTE_TICK_MS)) {
            m_tenMinuteAccum -= TEN_MINUTE_TICK_MS;
            tenMinuteTick();
        }

        // Inner loop: process map ticks until 100ms has elapsed (matches binary)
        int iterations = 0;
        do {
            processMapThreads();

            clock_t innerNow = clock();
            int elapsedMs = static_cast<int>(
                (innerNow - loopStart) * 1000 / CLOCKS_PER_SEC);
            if (elapsedMs >= TICK_INTERVAL_MS) break;

            // Sleep for remaining time (FUN_00447ca0 in binary)
            int remainMs = TICK_INTERVAL_MS - elapsedMs;
            if (remainMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            iterations++;
        } while (true);

        // CTRL+E exit check (binary: sf::Keyboard::isKeyPressed(0x25=LControl) && isKeyPressed(4=E))
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) &&
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
            LOG_INFO("CTRL+E pressed, initiating shutdown");
            m_running.store(false);
        }
    }

    LOG_INFO("Server main loop exited");
}

void Server::shutdown() {
    if (!m_running.load() && m_maps.empty()) {
        return; // Already shut down
    }

    LOG_INFO("Server shutting down...");
    m_running.store(false);

    // Save all online players
    PlayerMgr::instance().saveAllPlayers();

    // Disconnect all sessions
    if (m_networkMgr) {
        m_networkMgr->shutdown();
    }

    // Flush async database queries
    DatabaseMgr::instance().shutdown();

    // Clear map instances
    m_maps.clear();

    m_gameCache.reset();
    m_networkMgr.reset();

    Log::flush();
    LOG_INFO("Server shutdown complete");
}

void Server::scheduleShutdown(int seconds) {
    if (seconds <= 0) {
        LOG_INFO("Immediate shutdown requested");
        m_running.store(false);
        return;
    }

    LOG_INFO("Server shutdown scheduled in %d seconds", seconds);
    m_shutdownScheduled = true;
    m_shutdownCountdown = seconds;
    m_shutdownTickAccum = 0;
    m_lastBroadcastSecond = seconds + 1; // force immediate first broadcast

    broadcastShutdownMessage(seconds);
}

ServerMap* Server::getMap(uint32 mapId) const {
    auto it = m_maps.find(mapId);
    if (it != m_maps.end()) {
        return it->second.get();
    }
    return nullptr;
}

// --- Per-tick processing ---

void Server::processMapThreads() {
    for (auto& [id, map] : m_maps) {
        map->update(m_deltaTime);
    }
}

void Server::processPlayerConnections() {
    if (m_networkMgr) {
        m_networkMgr->acceptConnections();
        m_networkMgr->processNetworkIO(
            static_cast<float>(m_deltaTime) / 1000.0f);
    }

    // Process async DB callbacks on main thread
    DatabaseMgr::instance().processCallbacks();
}

void Server::processTimedEvents() {
    // Update arena matchmaking and active arenas
    ArenaMgr::instance().update(static_cast<int32>(m_deltaTime));
}

// --- Periodic tasks ---

void Server::minuteTick() {
    LOG_DEBUG("MinuteTick fired (uptime: %u ms)", m_msTime);

    // Save all online players (binary: FUN_0045e9e0)
    PlayerMgr::instance().saveAllPlayers();
}

void Server::tenMinuteTick() {
    LOG_DEBUG("TenMinuteTick fired (uptime: %u ms)", m_msTime);

    // Clean up stale sessions, respawn rare spawns (binary: FUN_0045fac0)
    LOG_INFO("Online: %u players, %zu maps active",
             PlayerMgr::instance().getPlayerCount(), m_maps.size());
}

// --- Shutdown countdown ---

void Server::updateShutdownCountdown() {
    m_shutdownTickAccum += m_deltaTime;

    while (m_shutdownTickAccum >= 1000 && m_shutdownCountdown > 0) {
        m_shutdownTickAccum -= 1000;
        m_shutdownCountdown--;

        int interval = getShutdownBroadcastInterval(m_shutdownCountdown);
        if (m_shutdownCountdown == 0 ||
            (m_lastBroadcastSecond - m_shutdownCountdown) >= interval) {
            broadcastShutdownMessage(m_shutdownCountdown);
            m_lastBroadcastSecond = m_shutdownCountdown;
        }
    }

    if (m_shutdownCountdown <= 0) {
        LOG_INFO("Shutdown countdown reached zero");
        m_running.store(false);
    }
}

int Server::getShutdownBroadcastInterval(int secondsRemaining) const {
    // Original binary broadcast schedule (from FUN_0045e440):
    //   < 10s  : every second
    //   < 60s  : every 15 seconds (0xf)
    //   < 600s : every 60 seconds (0x3c)
    //   >= 600s: every 300 seconds
    if (secondsRemaining < 10)  return 1;
    if (secondsRemaining < 60)  return 15;
    if (secondsRemaining < 600) return 60;
    return 300;
}

void Server::broadcastShutdownMessage(int secondsRemaining) {
    std::string msg;
    if (secondsRemaining <= 0) {
        msg = "Server is shutting down NOW";
    } else if (secondsRemaining < 60) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Server shutting down in %d second(s)", secondsRemaining);
        msg = buf;
    } else {
        int minutes = secondsRemaining / 60;
        char buf[128];
        snprintf(buf, sizeof(buf), "Server shutting down in %d minute(s)", minutes);
        msg = buf;
    }

    LOG_INFO("[SHUTDOWN] %s", msg.c_str());

    // Broadcast to all connected players (FUN_00460980 in binary)
    PlayerMgr::instance().broadcastSystemMessage(msg);
}
