#pragma once
#include "core/Types.h"
#include <string>

// Base class for all game entities (shared between client and server)
// Matches the original MutualObject.cpp hierarchy
class MutualObject {
public:
    virtual ~MutualObject() = default;

    Guid   getGuid() const { return m_guid; }
    uint32 getEntry() const { return m_entry; }
    uint32 getMapId() const { return m_mapId; }
    float  getPositionX() const { return m_posX; }
    float  getPositionY() const { return m_posY; }
    float  getOrientation() const { return m_orientation; }

    void setGuid(Guid guid) { m_guid = guid; }
    void setEntry(uint32 entry) { m_entry = entry; }
    void setMapId(uint32 mapId) { m_mapId = mapId; }
    void setPosition(float x, float y) { m_posX = x; m_posY = y; }
    void setOrientation(float o) { m_orientation = o; }

    float distanceTo(const MutualObject& other) const;
    float distanceTo(float x, float y) const;

protected:
    Guid   m_guid        = 0;
    uint32 m_entry       = 0;
    uint32 m_mapId       = 0;
    float  m_posX        = 0.0f;
    float  m_posY        = 0.0f;
    float  m_orientation = 0.0f;
};
