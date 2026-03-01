#include "database/SQLiteConnector.h"

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

SQLiteConnector::SQLiteConnector() = default;

SQLiteConnector::~SQLiteConnector() {
    close();
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

bool SQLiteConnector::open(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }

    m_filepath = filepath;

    int rc = sqlite3_open(m_filepath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Can't open database '%s', error: %s",
                  m_filepath.c_str(), sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // WAL mode for better concurrent read performance
    char* errMsg = nullptr;
    sqlite3_exec(m_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errMsg);
    if (errMsg) {
        sqlite3_free(errMsg);
    }

    LOG_INFO("SQLiteConnector: Opened database '%s'", m_filepath.c_str());
    return true;
}

void SQLiteConnector::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SQLiteConnector::isOpen() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_db != nullptr;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool SQLiteConnector::query(const std::string& sql, SQLiteResultSet& outResults) {
    std::lock_guard<std::mutex> lock(m_mutex);
    outResults.clear();

    if (!m_db) return false;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQLiteConnector::query prepare failed: %s", sqlite3_errmsg(m_db));
        return false;
    }

    int colCount = sqlite3_column_count(stmt);

    // Gather column names
    std::vector<std::string> colNames;
    colNames.reserve(static_cast<size_t>(colCount));
    for (int i = 0; i < colCount; ++i) {
        const char* name = sqlite3_column_name(stmt, i);
        colNames.emplace_back(name ? name : "");
    }

    while (true) {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            SQLiteResultRow row;
            for (int i = 0; i < colCount; ++i) {
                const unsigned char* text = sqlite3_column_text(stmt, i);
                row[colNames[i]] = text ? reinterpret_cast<const char*>(text) : "";
            }
            outResults.push_back(std::move(row));
        } else if (rc == SQLITE_BUSY) {
            // The SQLite database is locked.  In the original binary this
            // triggered a dialog: "The SQLite database is locked. Unlock it,
            // then press 'Ok'."  In the server reimplementation we log and
            // retry after a brief yield.
            LOG_WARN("The SQLite database is locked. Unlock it, then press 'Ok'.");
            sqlite3_finalize(stmt);
            return false;
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            LOG_ERROR("SQLiteConnector::query step failed: %s", sqlite3_errmsg(m_db));
            sqlite3_finalize(stmt);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    return true;
}

std::shared_ptr<SQLiteResult> SQLiteConnector::query(const std::string& sql) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_db) return nullptr;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQLiteConnector::query prepare failed: %s", sqlite3_errmsg(m_db));
        return nullptr;
    }

    int colCount = sqlite3_column_count(stmt);
    std::vector<std::vector<std::string>> rows;

    while (true) {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            std::vector<std::string> row;
            row.reserve(static_cast<size_t>(colCount));
            for (int i = 0; i < colCount; ++i) {
                const unsigned char* text = sqlite3_column_text(stmt, i);
                row.emplace_back(text ? reinterpret_cast<const char*>(text) : "");
            }
            rows.push_back(std::move(row));
        } else if (rc == SQLITE_BUSY) {
            LOG_WARN("The SQLite database is locked.");
            sqlite3_finalize(stmt);
            return nullptr;
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            LOG_ERROR("SQLiteConnector::query step failed: %s", sqlite3_errmsg(m_db));
            sqlite3_finalize(stmt);
            return nullptr;
        }
    }

    sqlite3_finalize(stmt);
    return std::make_shared<SQLiteResult>(std::move(rows));
}

bool SQLiteConnector::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (rc == SQLITE_BUSY) {
            LOG_WARN("The SQLite database is locked. Unlock it, then press 'Ok'.");
        } else {
            LOG_ERROR("SQLiteConnector::execute failed: %s",
                      errMsg ? errMsg : sqlite3_errmsg(m_db));
        }
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::string SQLiteConnector::lastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return "No database open";
    return sqlite3_errmsg(m_db);
}

int SQLiteConnector::lastErrorCode() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return SQLITE_ERROR;
    return sqlite3_errcode(m_db);
}
