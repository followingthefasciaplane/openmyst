#include "database/MySQLConnector.h"

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

MySQLConnector::MySQLConnector() = default;

MySQLConnector::~MySQLConnector() {
    disconnect();
}

MySQLConnector::MySQLConnector(MySQLConnector&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_mysql     = other.m_mysql;
    m_host      = std::move(other.m_host);
    m_port      = other.m_port;
    m_user      = std::move(other.m_user);
    m_password  = std::move(other.m_password);
    m_database  = std::move(other.m_database);
    m_connected = other.m_connected;
    other.m_mysql     = nullptr;
    other.m_connected = false;
}

MySQLConnector& MySQLConnector::operator=(MySQLConnector&& other) noexcept {
    if (this != &other) {
        disconnect();
        std::lock_guard<std::mutex> lockOther(other.m_mutex);
        std::lock_guard<std::mutex> lockSelf(m_mutex);
        m_mysql     = other.m_mysql;
        m_host      = std::move(other.m_host);
        m_port      = other.m_port;
        m_user      = std::move(other.m_user);
        m_password  = std::move(other.m_password);
        m_database  = std::move(other.m_database);
        m_connected = other.m_connected;
        other.m_mysql     = nullptr;
        other.m_connected = false;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// Connection management
// ---------------------------------------------------------------------------

bool MySQLConnector::connect(const std::string& host, uint16 port,
                             const std::string& user, const std::string& password,
                             const std::string& database) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_mysql) {
        mysql_close(m_mysql);
        m_mysql = nullptr;
    }

    m_host     = host;
    m_port     = port;
    m_user     = user;
    m_password = password;
    m_database = database;

    m_mysql = mysql_init(nullptr);
    if (!m_mysql) {
        LOG_ERROR("MySQLConnector: mysql_init failed");
        m_connected = false;
        return false;
    }

    // Enable auto-reconnect at the protocol level
    my_bool reconnect = 1;
    mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(m_mysql, m_host.c_str(), m_user.c_str(),
                            m_password.c_str(), m_database.c_str(),
                            m_port, nullptr, 0)) {
        LOG_ERROR("Can't connect to MySQL database '%s', error: %s",
                  m_database.c_str(), mysql_error(m_mysql));
        mysql_close(m_mysql);
        m_mysql = nullptr;
        m_connected = false;
        return false;
    }

    m_connected = true;
    LOG_INFO("MySQLConnector: Connected to '%s' on %s:%u",
             m_database.c_str(), m_host.c_str(), m_port);
    return true;
}

void MySQLConnector::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_mysql) {
        mysql_close(m_mysql);
        m_mysql = nullptr;
    }
    m_connected = false;
}

bool MySQLConnector::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected && m_mysql != nullptr;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool MySQLConnector::query(const std::string& sql, ResultSet& outResults) {
    std::lock_guard<std::mutex> lock(m_mutex);
    outResults.clear();

    if (!m_mysql) return false;

    if (mysql_query(m_mysql, sql.c_str()) != 0) {
        // Check for connection-lost errors (CR_SERVER_GONE_ERROR, CR_SERVER_LOST)
        unsigned int err = mysql_errno(m_mysql);
        if (err == 2006 || err == 2013) {
            LOG_WARN("MySQL: Connection lost during query, attempting reconnect...");
            if (!tryReconnect()) return false;
            // Retry the query once after reconnect
            if (mysql_query(m_mysql, sql.c_str()) != 0) {
                LOG_ERROR("MySQLConnector::query failed after reconnect: %s", mysql_error(m_mysql));
                return false;
            }
        } else {
            LOG_ERROR("MySQLConnector::query failed: %s", mysql_error(m_mysql));
            return false;
        }
    }

    // If the query returns no result set (e.g. INSERT), that's still success
    if (mysql_field_count(m_mysql) == 0)
        return true;

    MYSQL_RES* result = mysql_store_result(m_mysql);
    if (!result) {
        LOG_ERROR("MySQLConnector::query - mysql_store_result failed: %s", mysql_error(m_mysql));
        return false;
    }

    unsigned int numFields = mysql_num_fields(result);
    MYSQL_FIELD* fields = mysql_fetch_fields(result);

    // Build column name list
    std::vector<std::string> columnNames;
    columnNames.reserve(numFields);
    for (unsigned int i = 0; i < numFields; ++i) {
        columnNames.emplace_back(fields[i].name);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        ResultRow r;
        for (unsigned int i = 0; i < numFields; ++i) {
            r[columnNames[i]] = row[i] ? row[i] : "";
        }
        outResults.push_back(std::move(r));
    }

    mysql_free_result(result);
    return true;
}

bool MySQLConnector::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_mysql) return false;

    if (mysql_query(m_mysql, sql.c_str()) != 0) {
        unsigned int err = mysql_errno(m_mysql);
        if (err == 2006 || err == 2013) {
            LOG_WARN("MySQL: Connection lost during query, attempting reconnect...");
            if (!tryReconnect()) return false;
            if (mysql_query(m_mysql, sql.c_str()) != 0) {
                LOG_ERROR("MySQLConnector::execute failed after reconnect: %s",
                          mysql_error(m_mysql));
                return false;
            }
        } else {
            LOG_ERROR("MySQLConnector::execute failed: %s", mysql_error(m_mysql));
            return false;
        }
    }
    return true;
}

bool MySQLConnector::ping() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_mysql) return false;

    if (mysql_ping(m_mysql) != 0) {
        LOG_WARN("MySQLConnector::ping failed, attempting reconnect");
        return tryReconnect();
    }
    return true;
}

std::string MySQLConnector::escapeString(const std::string& input) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    // mysql_escape_string does not require a live connection
    std::string escaped;
    escaped.resize(input.size() * 2 + 1);
    unsigned long len = mysql_escape_string(escaped.data(), input.c_str(),
                                            static_cast<unsigned long>(input.size()));
    escaped.resize(len);
    return escaped;
}

std::string MySQLConnector::lastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_mysql) return "No connection";
    return mysql_error(m_mysql);
}

uint32 MySQLConnector::lastErrno() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_mysql) return 0;
    return static_cast<uint32>(mysql_errno(m_mysql));
}

// ---------------------------------------------------------------------------
// Reconnect
// ---------------------------------------------------------------------------

bool MySQLConnector::tryReconnect() {
    // Caller must already hold m_mutex
    LOG_INFO("MySQL: Attempting reconnection to '%s'...", m_database.c_str());

    if (m_mysql) {
        mysql_close(m_mysql);
        m_mysql = nullptr;
    }

    m_mysql = mysql_init(nullptr);
    if (!m_mysql) {
        LOG_ERROR("MySQL: Reconnection failed.");
        m_connected = false;
        return false;
    }

    my_bool reconnect = 1;
    mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(m_mysql, m_host.c_str(), m_user.c_str(),
                            m_password.c_str(), m_database.c_str(),
                            m_port, nullptr, 0)) {
        LOG_ERROR("MySQL: Reconnection failed.");
        mysql_close(m_mysql);
        m_mysql = nullptr;
        m_connected = false;
        return false;
    }

    LOG_INFO("MySQL: Reconnected successfully.");
    m_connected = true;
    return true;
}
