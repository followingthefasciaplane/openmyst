// Dreadmyst Server - Entry point
// Reimplemented from binary main at 0x00447560
//
// Startup sequence verified from decompilation:
//   1. timeBeginPeriod(1)                      -- raise timer resolution
//   2. SetUnhandledExceptionFilter              -- crash handler
//   3. Disable console close button             -- prevent accidental kills
//   4. FUN_0045e130 (Server::initialize)        -- load config, init DB, cache, guilds, arenas
//   5. Print "Dreadmyst r%d\n" banner
//   6. Print "Press CTRL+E to exit the program.\n"
//   7. Main loop:
//      a. Server tick (FUN_0045e440) -- timing, shutdown countdown, periodic tasks
//      b. Map ticks (FUN_004221b0)  -- NPC AI, respawn, etc.
//      c. Network I/O inner loop    -- spin up to 100ms processing packets
//      d. CTRL+E check (sf::Keyboard::isKeyPressed 0x25=LControl, 4=E)
//   8. On exit: disconnect all sessions, destruct subsystems
//   9. Print "Destructing...\n"

#include "core/Server.h"
#include "core/Config.h"
#include "core/Log.h"
#include "core/Types.h"
#include "database/DatabaseMgr.h"
#include "game/GameCache.h"
#include "game/Guild.h"
#include "game/Arena.h"
#include "network/NetworkMgr.h"

#include <SFML/Window/Keyboard.hpp>
#include <csignal>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
  #include <windows.h>
  #pragma comment(lib, "winmm.lib")
#endif

// Global reference for signal handler
static Server* g_server = nullptr;

static void signalHandler(int sig) {
    if (g_server) {
        printf("\nReceived signal %d, shutting down...\n", sig);
        g_server->shutdown();
    }
}

int main(int /*argc*/, char* /*argv*/[]) {
#ifdef _WIN32
    // Match binary: raise timer resolution to 1ms for accurate sleep/clock
    timeBeginPeriod(1);

    // Disable the close button on the console window (binary does this)
    HWND hWnd = GetConsoleWindow();
    if (hWnd) {
        HMENU hMenu = GetSystemMenu(hWnd, FALSE);
        EnableMenuItem(hMenu, SC_CLOSE, MF_GRAYED);
    }
#endif

    // Install signal handlers (Unix equivalent of SetUnhandledExceptionFilter)
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // ---- Load configuration ----
    Config& config = Config::instance();
    if (!config.load("config.ini")) {
        LOG_WARN("Could not load config.ini, using defaults");
    }

    // ---- Initialize logging ----
    std::string logFile = config.getString("Logging", "FilePath", "");
    bool logToFile = config.getBool("Logging", "FileEnabled", false);
    Log::init(logToFile ? logFile : "");
    Log::setLevel(Log::LOG_INFO);

    LOG_INFO("Dreadmyst Server starting...");

    // ---- Initialize database connections ----
    // DatabaseMgr::begin() reads MySQL and SQLite config from Config,
    // connects both databases, and starts async query worker threads.
    DatabaseMgr& db = DatabaseMgr::instance();
    if (!db.begin()) {
        LOG_FATAL("Failed to init database(s)");
        return 1;
    }

    LOG_INFO("Database connections established");

    // ---- Initialize server (loads GameCache, creates maps, starts network) ----
    // Server::initialize() handles:
    //   - Creating and loading GameCache from SQLite (all templates)
    //   - Creating PlayerMgr
    //   - Creating NetworkMgr and binding the listener port
    //   - Creating and starting map threads
    Server& server = Server::instance();
    g_server = &server;

    if (!server.initialize()) {
        fprintf(stderr, "Server initialization failed\n");
        LOG_FATAL("Failed to initialize server");
        return 1;
    }

    // ---- Load guilds from MySQL ----
    GuildMgr::instance().loadFromDB();
    LOG_INFO("Guilds loaded: %u", GuildMgr::instance().getGuildCount());

    // ---- Load arena templates ----
    ArenaMgr::instance().loadTemplates();
    LOG_INFO("Arena templates loaded");

    // Banner -- exact strings from binary
    printf("Dreadmyst r%d\n", DREADMYST_REVISION);
    printf("Press CTRL+E to exit the program.\n");

    LOG_INFO("Server initialized successfully");
    LOG_INFO("Listening on port %d", config.getInt("Tcp", "Port", DEFAULT_TCP_PORT));

    // Main game loop (matches binary at 0x00447560)
    // The binary loop structure:
    //   while (!shutdownFlag) {
    //     clock_t loopStart = clock();
    //     serverTick(deltaTime);            // FUN_0045e440
    //     do {
    //       mapTick();                      // FUN_004221b0
    //       networkSleep(remaining);        // FUN_00447ca0
    //     } while (clock() - loopStart < 100);
    //     if (CTRL pressed && E pressed) shutdown = true;
    //   }
    server.run();

    // Shutdown and cleanup (matches binary exit path)
    printf("Destructing...\n");
    LOG_INFO("Server shutting down...");
    server.shutdown();

#ifdef _WIN32
    timeEndPeriod(1);
#endif

    LOG_INFO("Server stopped.");
    return 0;
}
