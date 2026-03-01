#pragma once

#include "GameDefines.h"
#include <string>
#include <vector>

// Base class for all game entities shared between client and server.
// Binary RTTI: .?AVMutualObject@@ at 0x0067ef5c (client), 0x005c180c (server)
// Source: C:\Software Development\Dreadmyst\Github\Shared\MutualObject.cpp
//
// Binary layout (as sub-object at offset +0x40 in ClientObject):
//   +0x00: vtable*
//   +0x04: m_guid (int32)
//   +0x08: m_type (ObjectType enum, 0=None, 1=GameObj, 2=Player, 3=Npc)
//   +0x0C: m_name (std::string, 24 bytes SSO)
//   +0x24: m_subname (std::string, 24 bytes SSO)
//   +0x3C: m_variables (vector<int>, 12 bytes: begin/end/capacity)
//
// All stats (health, mana, level, etc.) are stored in m_variables, indexed
// by ObjDefines::Variable enum. Confirmed by decompilation of the
// ObjectVariable handler at 0x00466030:
//   *(int*)(object+0x7C)[varId] = value;  // +0x7C = ClientObject base + 0x3C
//   vtable[5](varId, value);              // notifyVariableChange
//
// Note: Position is stored in WorldRenderable (client) or ServerUnit (server),
// NOT in MutualObject.
class MutualObject
{
public:
    virtual ~MutualObject() = default;

    int getGuid() const { return m_guid; }
    ObjDefines::ObjectType getType() const { return m_type; }

    void setGuid(int guid) { m_guid = guid; }

    bool isPlayer() const { return m_type == ObjDefines::Type_Player; }
    bool isNpc() const { return m_type == ObjDefines::Type_Npc; }
    bool isGameObject() const { return m_type == ObjDefines::Type_GameObject; }

    // Notify when a server variable changes for this object
    virtual void notifyVariableChange(const ObjDefines::Variable var, const int value) {}
    virtual void notifyNameChanged() {}
    virtual void notifySubnameChanged() {}
    virtual int getEntry() const { return 0; }

    const std::string& getName() const { return m_name; }
    const std::string& getSubname() const { return m_subname; }
    void setName(const std::string& name) { m_name = name; notifyNameChanged(); }
    void setSubname(const std::string& sub) { m_subname = sub; notifySubnameChanged(); }

    // Variable system — all stats stored in vector<int>, indexed by Variable enum.
    // Wire format: variableId is uint8_t (max 255 variables).
    int getVariable(ObjDefines::Variable var) const
    {
        const auto idx = static_cast<size_t>(var);
        return (idx < m_variables.size()) ? m_variables[idx] : 0;
    }

    void setVariable(ObjDefines::Variable var, int value)
    {
        const auto idx = static_cast<size_t>(var);
        if (idx >= m_variables.size())
            m_variables.resize(idx + 1, 0);
        if (m_variables[idx] != value)
        {
            m_variables[idx] = value;
            notifyVariableChange(var, value);
        }
    }

    // Convenience stat accessors (all delegate to variable vector)
    int getHealth() const { return getVariable(ObjDefines::Health); }
    int getMaxHealth() const { return getVariable(ObjDefines::MaxHealth); }
    int getMana() const { return getVariable(ObjDefines::Mana); }
    int getMaxMana() const { return getVariable(ObjDefines::MaxMana); }
    int getLevel() const { return getVariable(ObjDefines::Level); }
    int getExperience() const { return getVariable(ObjDefines::Experience); }
    int getGold() const { return getVariable(ObjDefines::Gold); }
    int getDeathState() const { return getVariable(ObjDefines::DeathState); }
    bool isAlive() const { return getDeathState() == 0; }
    bool isDead() const { return getDeathState() != 0; }

protected:
    // Binary assertion in initType: "m_type == Type::Error" (line 0xBE = 190)
    // Type must be set exactly once via initType() in the subclass constructor.
    void initType(ObjDefines::ObjectType type) { m_type = type; }

    int m_guid = 0;
    ObjDefines::ObjectType m_type = ObjDefines::Type_None;
    std::string m_name;
    std::string m_subname;
    std::vector<int> m_variables;
};
