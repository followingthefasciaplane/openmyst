#pragma once
#include "core/Types.h"
#include "core/Log.h"

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <memory>
#include <cstdlib>

// Reuse the same result types as the MySQL connector for uniformity
using SQLiteResultRow = std::map<std::string, std::string>;
using SQLiteResultSet = std::vector<SQLiteResultRow>;

// Cursor-style result wrapper used by GameCache
class SQLiteResult {
public:
    SQLiteResult(std::vector<std::vector<std::string>>&& rows)
        : m_rows(std::move(rows)), m_currentRow(-1) {}

    bool next() {
        ++m_currentRow;
        return m_currentRow < static_cast<int>(m_rows.size());
    }

    uint32 getUint32(int col) const {
        return static_cast<uint32>(std::strtoul(m_rows[m_currentRow][col].c_str(), nullptr, 10));
    }
    int32 getInt32(int col) const {
        return static_cast<int32>(std::strtol(m_rows[m_currentRow][col].c_str(), nullptr, 10));
    }
    float getFloat(int col) const {
        return std::strtof(m_rows[m_currentRow][col].c_str(), nullptr);
    }
    std::string getString(int col) const {
        return m_rows[m_currentRow][col];
    }

private:
    std::vector<std::vector<std::string>> m_rows;
    int m_currentRow;
};

// SQLite database connector for game template data.
// Opens a local game.db file and provides thread-safe query access.
// The original binary embedded SQLite 3.30.1.
class SQLiteConnector {
public:
    SQLiteConnector();
    ~SQLiteConnector();

    // Non-copyable
    SQLiteConnector(const SQLiteConnector&) = delete;
    SQLiteConnector& operator=(const SQLiteConnector&) = delete;

    // Open a SQLite database file.  Returns true on success.
    bool open(const std::string& filepath);

    // Close the database handle.
    void close();

    // Returns true when a database is open.
    bool isOpen() const;

    // Execute a SELECT-style query and fill outResults.
    bool query(const std::string& sql, SQLiteResultSet& outResults);

    // Cursor-style query returning a result object (used by GameCache)
    std::shared_ptr<SQLiteResult> query(const std::string& sql);

    // Execute a statement that produces no result set.
    bool execute(const std::string& sql);

    // Last error message from SQLite.
    std::string lastError() const;

    // Last error code.
    int lastErrorCode() const;

    // Underlying handle.
    sqlite3* handle() { return m_db; }

private:
    sqlite3*    m_db = nullptr;
    std::string m_filepath;
    mutable std::mutex m_mutex;
};
