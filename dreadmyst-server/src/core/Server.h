#pragma once
#include "core/Types.h"
#include "core/Log.h"
#include "core/Config.h"
#include "database/DatabaseMgr.h"

#include <ctime>
#include <atomic>
#include <memory>
#include <map>
#include <string>

// Forward declarations
class NetworkMgr;
class DatabaseMgr;
class GameCache;
class PlayerMgr;
class ServerMap;

// Main server singleton - reimplementation of original at 0x0040d260
// Drives the game loop at 10 ticks/second (100ms intervals)
// Shutdown flag originally at 0x005c3b00
class Server {
public:
    static Server& instance();

    // Lifecycle
    bool initialize();
    void run();
    void shutdown();

    // Scheduled shutdown with broadcast countdown
    // Broadcasts at intervals: <10s every 1s, <60s every 15s, <600s every 60s, >=600s every 300s
    void scheduleShutdown(int seconds);

    // Timing accessors (updated each tick)
    uint32 getDeltaTime()  const { return m_deltaTime; }
    clock_t getTickClock() const { return m_tickClock; }
    int64  getUnixTime()   const { return m_unixTime; }
    uint32 getMsTime()     const { return m_msTime; }

    // Subsystem accessors
    NetworkMgr*  getNetworkMgr()  const { return m_networkMgr.get(); }
    DatabaseMgr& getDatabaseMgr() const { return DatabaseMgr::instance(); }
    GameCache*   getGameCache()   const { return m_gameCache.get(); }
    PlayerMgr*   getPlayerMgr()   const { return m_playerMgr.get(); }

    ServerMap* getMap(uint32 mapId) const;
    const std::map<uint32, std::unique_ptr<ServerMap>>& getMaps() const { return m_maps; }

    bool isRunning() const { return m_running.load(); }

private:
    Server();
    ~Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // Per-tick processing (called from main loop)
    void processMapThreads();
    void processPlayerConnections();
    void processTimedEvents();

    // Periodic tasks
    void minuteTick();
    void tenMinuteTick();

    // Shutdown countdown logic
    void updateShutdownCountdown();
    void broadcastShutdownMessage(int secondsRemaining);
    int  getShutdownBroadcastInterval(int secondsRemaining) const;

    // Timing state
    uint32  m_deltaTime  = 0;       // ms elapsed since last tick
    clock_t m_tickClock  = 0;       // clock() value at current tick
    int64   m_unixTime   = 0;       // _time64 unix timestamp
    uint32  m_msTime     = 0;       // running ms counter

    // Periodic task accumulators
    uint32 m_minuteAccum     = 0;
    uint32 m_tenMinuteAccum  = 0;

    // Shutdown state
    std::atomic<bool> m_running{false};
    bool   m_shutdownScheduled     = false;
    int    m_shutdownCountdown     = 0;   // seconds remaining
    uint32 m_shutdownTickAccum     = 0;   // ms accumulator for 1-second countdown
    int    m_lastBroadcastSecond   = -1;  // last second at which we broadcast

    // Subsystem pointers
    std::unique_ptr<NetworkMgr>  m_networkMgr;
    std::unique_ptr<GameCache>   m_gameCache;
    std::unique_ptr<PlayerMgr>   m_playerMgr;

    // Map instances keyed by map ID
    std::map<uint32, std::unique_ptr<ServerMap>> m_maps;
};
