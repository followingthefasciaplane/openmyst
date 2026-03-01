#pragma once

#include <cstdint>

// Map cell base class for spatial partitioning.
// Specialized by MapCellClient (client) and MapCell (server).
class MapCellT
{
public:
    // Cell property flags
    enum CellFlags : int
    {
        None        = 0x00,
        Unwalkable  = 0x01,
        Blocking    = 0x02,
        Occupied    = 0x04,
    };

    virtual ~MapCellT() = default;

    int getCellId() const { return m_cellId; }
    void setCellId(int id) { m_cellId = id; }

    int getFlags() const { return m_flags; }
    void setFlags(int flags) { m_flags = flags; }
    void addFlag(int flag) { m_flags |= flag; }
    bool hasFlag(int flag) const { return (m_flags & flag) != 0; }

    bool isUnwalkable() const { return hasFlag(Unwalkable); }
    bool isBlocking() const { return hasFlag(Blocking); }

protected:
    int m_cellId = 0;
    int m_flags  = 0;
};
