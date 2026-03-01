#pragma once
#include "core/Types.h"
#include "network/Opcodes.h"

#include <SFML/Network.hpp>
#include <string>
#include <vector>
#include <cstring>

// ============================================================================
//  PacketBuffer - Typed read/write wrapper over raw byte data
// ============================================================================
// SFML packet wire format: [4 bytes size (big-endian uint32)] [N bytes data]
// Within data: [2 bytes opcode (uint16)] [payload...]
//
// This class provides structured access to the payload portion, with typed
// read/write methods matching the original client/server serialization.
// ============================================================================

class PacketBuffer {
public:
    PacketBuffer();
    explicit PacketBuffer(uint16 opcode);
    PacketBuffer(const uint8* data, size_t size);
    PacketBuffer(const PacketBuffer&) = default;
    PacketBuffer& operator=(const PacketBuffer&) = default;
    PacketBuffer(PacketBuffer&&) noexcept = default;
    PacketBuffer& operator=(PacketBuffer&&) noexcept = default;
    ~PacketBuffer() = default;

    // --- Write methods (append to buffer) ---
    void writeUint8(uint8 value);
    void writeUint16(uint16 value);
    void writeUint32(uint32 value);
    void writeInt8(int8 value);
    void writeInt16(int16 value);
    void writeInt32(int32 value);
    void writeFloat(float value);
    void writeString(const std::string& value);
    void writeBytes(const uint8* data, size_t len);

    // --- Read methods (advance internal read cursor) ---
    uint8       readUint8();
    uint16      readUint16();
    uint32      readUint32();
    int8        readInt8();
    int16       readInt16();
    int32       readInt32();
    float       readFloat();
    std::string readString();
    void        readBytes(uint8* dest, size_t len);

    // --- Opcode ---
    uint16 getOpcode() const { return m_opcode; }
    void   setOpcode(uint16 opcode) { m_opcode = opcode; }

    // --- Buffer access ---
    const uint8* data() const { return m_buffer.data(); }
    size_t       size() const { return m_buffer.size(); }
    size_t       readPos() const { return m_readPos; }
    size_t       remaining() const { return m_buffer.size() - m_readPos; }
    bool         empty() const { return m_buffer.empty(); }

    // Reset read cursor to beginning
    void resetRead() { m_readPos = 0; }

    // Clear all data and reset cursor
    void clear();

    // --- sf::Packet conversion ---
    // Pack opcode + payload into an sf::Packet for sending over the wire.
    // Wire format: sf::Packet internally prepends a 4-byte big-endian size.
    // Our data written into it: [uint16 opcode][payload bytes...]
    sf::Packet toSfPacket() const;

    // Parse from raw data received via sf::Packet.
    // Expects the data to begin with a uint16 opcode followed by payload.
    static PacketBuffer fromSfPacket(sf::Packet& sfPacket);

private:
    uint16              m_opcode  = 0;
    std::vector<uint8>  m_buffer;
    size_t              m_readPos = 0;
};

// ============================================================================
//  GamePacket - High-level packet with opcode, used for send/receive
// ============================================================================
// Thin wrapper that pairs an opcode with a PacketBuffer for convenience.
// Most code should use this rather than raw PacketBuffer.
// ============================================================================

class GamePacket {
public:
    GamePacket() = default;
    explicit GamePacket(uint16 opcode);
    GamePacket(uint16 opcode, PacketBuffer&& buffer);
    ~GamePacket() = default;

    GamePacket(const GamePacket&) = default;
    GamePacket& operator=(const GamePacket&) = default;
    GamePacket(GamePacket&&) noexcept = default;
    GamePacket& operator=(GamePacket&&) noexcept = default;

    // Opcode access
    uint16 getOpcode() const { return m_buffer.getOpcode(); }
    const char* getOpcodeName() const { return opcodeToString(getOpcode()); }

    // Delegate typed writes
    void writeUint8(uint8 v)               { m_buffer.writeUint8(v); }
    void writeUint16(uint16 v)             { m_buffer.writeUint16(v); }
    void writeUint32(uint32 v)             { m_buffer.writeUint32(v); }
    void writeInt8(int8 v)                 { m_buffer.writeInt8(v); }
    void writeInt16(int16 v)               { m_buffer.writeInt16(v); }
    void writeInt32(int32 v)               { m_buffer.writeInt32(v); }
    void writeFloat(float v)               { m_buffer.writeFloat(v); }
    void writeString(const std::string& v) { m_buffer.writeString(v); }
    void writeBytes(const uint8* d, size_t n) { m_buffer.writeBytes(d, n); }

    // Delegate typed reads
    uint8       readUint8()    { return m_buffer.readUint8(); }
    uint16      readUint16()   { return m_buffer.readUint16(); }
    uint32      readUint32()   { return m_buffer.readUint32(); }
    int8        readInt8()     { return m_buffer.readInt8(); }
    int16       readInt16()    { return m_buffer.readInt16(); }
    int32       readInt32()    { return m_buffer.readInt32(); }
    float       readFloat()    { return m_buffer.readFloat(); }
    std::string readString()   { return m_buffer.readString(); }
    void        readBytes(uint8* d, size_t n) { m_buffer.readBytes(d, n); }

    // Buffer access
    PacketBuffer&       buffer()       { return m_buffer; }
    const PacketBuffer& buffer() const { return m_buffer; }
    size_t              size()   const { return m_buffer.size(); }
    void                resetRead()    { m_buffer.resetRead(); }
    void                clear()        { m_buffer.clear(); }

    // Convert to sf::Packet for wire transmission
    sf::Packet toSfPacket() const { return m_buffer.toSfPacket(); }

private:
    PacketBuffer m_buffer;
};
