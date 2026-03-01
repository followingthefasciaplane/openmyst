#pragma once

#include <string>
#include <memory>

class QueryResult;

// SQLite database connector for client-side content databases
class SqlConnector
{
public:
    static SqlConnector& instance();

    bool open(const std::string& dbPath);
    void close();
    bool isOpen() const;

    // Execute a query and return results
    std::shared_ptr<QueryResult> query(const std::string& sql);

    // Execute a statement (no results)
    bool execute(const std::string& sql);

private:
    SqlConnector() = default;
    ~SqlConnector();

    void* m_db = nullptr;  // sqlite3*
};

#define sSql SqlConnector::instance()
