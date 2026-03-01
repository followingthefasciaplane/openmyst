#pragma once
#include "core/Types.h"
#include "core/Log.h"
#include "database/MySQLConnector.h"
#include "database/SQLiteConnector.h"
#include "database/QueuedQuery.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

// Identifies a logical database connection managed by DatabaseMgr.
enum class DbConnection : uint8 {
    Player = 0,   // MySQL - player data (accounts, characters, inventory, etc.)
    GameDb = 1    // SQLite - game template data (items, npcs, spells, quests, etc.)
};

// Central database manager.
//
// Manages both the MySQL connection (player data) and the SQLite connection
// (game template data loaded from game.db).  Provides queued worker threads
// for asynchronous queries against MySQL.
//
// Matches the original DatabaseMgr.cpp with begin(), getWorkerForDb().
class DatabaseMgr {
public:
    static DatabaseMgr& instance();

    // Initialize and connect all databases using settings from Config.
    // Returns true if every required database was initialized.
    // Logs "Failed to init database(s)" on failure.
    bool begin();

    // Shut down worker threads and close all connections.
    void shutdown();

    // Obtain the MySQL connector (player database).
    MySQLConnector& getPlayerConnection() { return m_playerDb; }
    const MySQLConnector& getPlayerConnection() const { return m_playerDb; }

    // Obtain the SQLite connector (game template database).
    SQLiteConnector& getGameDbConnection() { return m_gameDb; }
    const SQLiteConnector& getGameDbConnection() const { return m_gameDb; }

    // Get the async query worker for a specific logical database.
    // Only MySQL databases have workers; requesting a worker for SQLite
    // will log an error and return nullptr.
    // Error: "DatabaseMgr::getWorkerForDb - Unknown database connection!"
    QueuedQuery* getWorkerForDb(DbConnection db);

    // Convenience: process all pending callbacks for every worker.
    // Call once per server tick from the main thread.
    void processCallbacks();

private:
    DatabaseMgr() = default;
    ~DatabaseMgr();

    // Non-copyable
    DatabaseMgr(const DatabaseMgr&) = delete;
    DatabaseMgr& operator=(const DatabaseMgr&) = delete;

    MySQLConnector  m_playerDb;
    SQLiteConnector m_gameDb;

    // One worker thread per MySQL database
    QueuedQuery     m_playerWorker;

    bool m_initialized = false;
};
