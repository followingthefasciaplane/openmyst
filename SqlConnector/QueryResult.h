#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Query result row accessor
class QueryResult
{
public:
    QueryResult() = default;
    ~QueryResult() = default;

    // Navigate rows
    bool next();
    void reset();

    // Get column values by index (0-based)
    int getInt(int column) const;
    float getFloat(int column) const;
    std::string getString(int column) const;
    bool getBool(int column) const;

    // Get column values by name
    int getInt(const std::string& column) const;
    float getFloat(const std::string& column) const;
    std::string getString(const std::string& column) const;

    // Row/column info
    int getRowCount() const { return static_cast<int>(m_rows.size()); }
    int getColumnCount() const { return static_cast<int>(m_columnNames.size()); }
    bool empty() const { return m_rows.empty(); }

    // Internal: used by SqlConnector to populate results
    void addColumn(const std::string& name) { m_columnNames.push_back(name); }
    void addRow(const std::vector<std::string>& row) { m_rows.push_back(row); }

private:
    int getColumnIndex(const std::string& name) const;

    std::vector<std::string> m_columnNames;
    std::vector<std::vector<std::string>> m_rows;
    int m_currentRow = -1;
};
