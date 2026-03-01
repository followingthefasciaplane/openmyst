#pragma once
#include "core/Types.h"
#include "core/Log.h"

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <mutex>
#include <functional>

// Result row: column name -> string value
using ResultRow = std::map<std::string, std::string>;
using ResultSet = std::vector<ResultRow>;

// Callback invoked with the result set after a query completes
using QueryCallback = std::function<void(ResultSet&&)>;

// MySQL database connector matching the original .?AVMySQLConnector@@ RTTI.
// Wraps libmysql and provides auto-reconnect, thread-safe query execution,
// and escaped query building.
class MySQLConnector {
public:
    MySQLConnector();
    ~MySQLConnector();

    // Non-copyable, movable
    MySQLConnector(const MySQLConnector&) = delete;
    MySQLConnector& operator=(const MySQLConnector&) = delete;
    MySQLConnector(MySQLConnector&& other) noexcept;
    MySQLConnector& operator=(MySQLConnector&& other) noexcept;

    // Connect to a MySQL server.  Returns true on success.
    bool connect(const std::string& host, uint16 port,
                 const std::string& user, const std::string& password,
                 const std::string& database);

    // Disconnect and release the handle.
    void disconnect();

    // Returns true when connected (or reconnectable).
    bool isConnected() const;

    // Execute a query and return the full result set.
    // Returns true on success.
    bool query(const std::string& sql, ResultSet& outResults);

    // Execute a query that does not return rows (INSERT / UPDATE / DELETE).
    // Returns true on success.
    bool execute(const std::string& sql);

    // Ping the server.  Triggers auto-reconnect if the connection was lost.
    bool ping();

    // Escape a string for safe embedding in SQL literals.
    std::string escapeString(const std::string& input) const;

    // Last error message / error number from MySQL.
    std::string lastError() const;
    uint32 lastErrno() const;

    // Underlying handle (for advanced usage / diagnostics).
    MYSQL* handle() { return m_mysql; }

private:
    // Attempt to reconnect after a lost connection.
    bool tryReconnect();

    MYSQL*      m_mysql       = nullptr;
    std::string m_host;
    uint16      m_port        = 3306;
    std::string m_user;
    std::string m_password;
    std::string m_database;
    bool        m_connected   = false;
    mutable std::mutex m_mutex;
};
