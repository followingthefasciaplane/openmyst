#pragma once

#include "UnitDefines.h"
#include "GameDefines.h"
#include "..\Geo2d.h"

// Shared unit interface for players and NPCs (client and server).
// Binary RTTI: .?AVMutualUnit@@ at 0x006815c0 (client), 0x005c1af0 (server)
//
// Provides unit-specific virtual methods (emitsLight, setCurrentCell)
// and orientation storage. Stats (health, mana, level, etc.) are stored in
// MutualObject::m_variables and accessed via MutualObject::getVariable().
//
// In ClientUnit, MutualUnit is the second base class in multiple inheritance:
//   class ClientUnit : public ClientObject, public MutualUnit
// MutualUnit sub-object is at ClientObject + 0xB0.
//
// Binary layout (as sub-object within ClientUnit):
//   +0x00 (obj+0xB0): vtable* (destructor, emitsLight, setCurrentCell)
//   +0x04 (obj+0xB4): m_posRef (const Geo2d::Vector2*)
//   +0x08 (obj+0xB8): unknown field (4 bytes)
//   +0x0C (obj+0xBC): m_orientation (float, confirmed by UnitOrientation handler at 0x00469d50)
class MutualUnit
{
public:
    virtual ~MutualUnit() = default;

    // Light emission for map rendering
    virtual bool emitsLight(void* ls = nullptr) { return false; }

    // Cell tracking for map-based spatial partitioning
    virtual void setCurrentCell(const int c, void* cell) {}

    float getOrientation() const { return m_orientation; }
    void setOrientation(float o) { m_orientation = o; }

protected:
    MutualUnit() = default;
    explicit MutualUnit(const Geo2d::Vector2& posRef) : m_posRef(&posRef) {}

    const Geo2d::Vector2* m_posRef = nullptr;
    int   m_unk08       = 0;       // +0x08: unknown, needs investigation
    float m_orientation = 0.0f;    // +0x0C: facing direction (radians)
};
