#include "database/DatabaseMgr.h"
#include "core/Config.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

DatabaseMgr& DatabaseMgr::instance() {
    static DatabaseMgr inst;
    return inst;
}

DatabaseMgr::~DatabaseMgr() {
    shutdown();
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

bool DatabaseMgr::begin() {
    LOG_INFO("DatabaseMgr::begin");

    auto& cfg = Config::instance();

    // -----------------------------------------------------------------------
    // MySQL: player database
    // -----------------------------------------------------------------------
    std::string mysqlHost = cfg.getString("MySQL", "Host", "127.0.0.1");
    uint16 mysqlPort      = static_cast<uint16>(cfg.getInt("MySQL", "Port", 3306));
    std::string mysqlUser = cfg.getString("MySQL", "User", "root");
    std::string mysqlPass = cfg.getString("MySQL", "Password", "");
    std::string mysqlDb   = cfg.getString("MySQL", "Database", "dreadmyst");

    if (!m_playerDb.connect(mysqlHost, mysqlPort, mysqlUser, mysqlPass, mysqlDb)) {
        LOG_FATAL("Failed to init database(s)");
        return false;
    }

    // Start the async worker for the player database
    m_playerWorker.start(&m_playerDb);

    // -----------------------------------------------------------------------
    // SQLite: game template database
    // -----------------------------------------------------------------------
    std::string gameDbPath = cfg.getString("SQLite", "DatabaseFile", "game.db");

    if (!m_gameDb.open(gameDbPath)) {
        LOG_FATAL("Failed to init database(s)");
        return false;
    }

    m_initialized = true;
    LOG_INFO("DatabaseMgr: All databases initialized successfully");
    return true;
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void DatabaseMgr::shutdown() {
    if (!m_initialized) return;

    LOG_INFO("DatabaseMgr: Shutting down...");

    m_playerWorker.stop();
    m_playerDb.disconnect();
    m_gameDb.close();

    m_initialized = false;
    LOG_INFO("DatabaseMgr: Shutdown complete");
}

// ---------------------------------------------------------------------------
// Worker access
// ---------------------------------------------------------------------------

QueuedQuery* DatabaseMgr::getWorkerForDb(DbConnection db) {
    switch (db) {
        case DbConnection::Player:
            return &m_playerWorker;

        case DbConnection::GameDb:
            // SQLite is synchronous; no worker thread.
            LOG_ERROR("DatabaseMgr::getWorkerForDb - Unknown database connection!");
            return nullptr;

        default:
            LOG_ERROR("DatabaseMgr::getWorkerForDb - Unknown database connection!");
            return nullptr;
    }
}

// ---------------------------------------------------------------------------
// Main-thread callback delivery
// ---------------------------------------------------------------------------

void DatabaseMgr::processCallbacks() {
    m_playerWorker.processCallbacks();
}
