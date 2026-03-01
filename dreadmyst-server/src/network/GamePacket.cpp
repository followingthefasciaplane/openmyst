#include "network/GamePacket.h"
#include "core/Log.h"

#include <stdexcept>
#include <cstring>

// ============================================================================
//  PacketBuffer implementation
// ============================================================================

PacketBuffer::PacketBuffer()
    : m_opcode(0), m_readPos(0) {
}

PacketBuffer::PacketBuffer(uint16 opcode)
    : m_opcode(opcode), m_readPos(0) {
}

PacketBuffer::PacketBuffer(const uint8* data, size_t size)
    : m_opcode(0), m_readPos(0) {
    if (data && size > 0) {
        m_buffer.assign(data, data + size);
    }
}

void PacketBuffer::clear() {
    m_buffer.clear();
    m_readPos = 0;
}

// --- Write methods ---

void PacketBuffer::writeUint8(uint8 value) {
    m_buffer.push_back(value);
}

void PacketBuffer::writeUint16(uint16 value) {
    m_buffer.push_back(static_cast<uint8>(value & 0xFF));
    m_buffer.push_back(static_cast<uint8>((value >> 8) & 0xFF));
}

void PacketBuffer::writeUint32(uint32 value) {
    m_buffer.push_back(static_cast<uint8>(value & 0xFF));
    m_buffer.push_back(static_cast<uint8>((value >> 8) & 0xFF));
    m_buffer.push_back(static_cast<uint8>((value >> 16) & 0xFF));
    m_buffer.push_back(static_cast<uint8>((value >> 24) & 0xFF));
}

void PacketBuffer::writeInt8(int8 value) {
    writeUint8(static_cast<uint8>(value));
}

void PacketBuffer::writeInt16(int16 value) {
    writeUint16(static_cast<uint16>(value));
}

void PacketBuffer::writeInt32(int32 value) {
    writeUint32(static_cast<uint32>(value));
}

void PacketBuffer::writeFloat(float value) {
    uint32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    writeUint32(bits);
}

void PacketBuffer::writeString(const std::string& value) {
    // Wire format: uint16 length + raw bytes (no null terminator)
    uint16 len = static_cast<uint16>(value.size());
    writeUint16(len);
    if (len > 0) {
        writeBytes(reinterpret_cast<const uint8*>(value.data()), len);
    }
}

void PacketBuffer::writeBytes(const uint8* data, size_t len) {
    if (data && len > 0) {
        m_buffer.insert(m_buffer.end(), data, data + len);
    }
}

// --- Read methods ---

uint8 PacketBuffer::readUint8() {
    if (m_readPos + 1 > m_buffer.size()) {
        LOG_ERROR("PacketBuffer::readUint8 - read past end (pos=%zu, size=%zu, opcode=0x%04X)",
                  m_readPos, m_buffer.size(), m_opcode);
        return 0;
    }
    return m_buffer[m_readPos++];
}

uint16 PacketBuffer::readUint16() {
    if (m_readPos + 2 > m_buffer.size()) {
        LOG_ERROR("PacketBuffer::readUint16 - read past end (pos=%zu, size=%zu, opcode=0x%04X)",
                  m_readPos, m_buffer.size(), m_opcode);
        return 0;
    }
    uint16 value = static_cast<uint16>(m_buffer[m_readPos])
                 | (static_cast<uint16>(m_buffer[m_readPos + 1]) << 8);
    m_readPos += 2;
    return value;
}

uint32 PacketBuffer::readUint32() {
    if (m_readPos + 4 > m_buffer.size()) {
        LOG_ERROR("PacketBuffer::readUint32 - read past end (pos=%zu, size=%zu, opcode=0x%04X)",
                  m_readPos, m_buffer.size(), m_opcode);
        return 0;
    }
    uint32 value = static_cast<uint32>(m_buffer[m_readPos])
                 | (static_cast<uint32>(m_buffer[m_readPos + 1]) << 8)
                 | (static_cast<uint32>(m_buffer[m_readPos + 2]) << 16)
                 | (static_cast<uint32>(m_buffer[m_readPos + 3]) << 24);
    m_readPos += 4;
    return value;
}

int8 PacketBuffer::readInt8() {
    return static_cast<int8>(readUint8());
}

int16 PacketBuffer::readInt16() {
    return static_cast<int16>(readUint16());
}

int32 PacketBuffer::readInt32() {
    return static_cast<int32>(readUint32());
}

float PacketBuffer::readFloat() {
    uint32 bits = readUint32();
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::string PacketBuffer::readString() {
    uint16 len = readUint16();
    if (len == 0) {
        return {};
    }
    if (m_readPos + len > m_buffer.size()) {
        LOG_ERROR("PacketBuffer::readString - string length %u exceeds remaining data "
                  "(pos=%zu, size=%zu, opcode=0x%04X)",
                  len, m_readPos, m_buffer.size(), m_opcode);
        m_readPos = m_buffer.size();
        return {};
    }
    std::string result(reinterpret_cast<const char*>(&m_buffer[m_readPos]), len);
    m_readPos += len;
    return result;
}

void PacketBuffer::readBytes(uint8* dest, size_t len) {
    if (m_readPos + len > m_buffer.size()) {
        LOG_ERROR("PacketBuffer::readBytes - read %zu bytes past end (pos=%zu, size=%zu, opcode=0x%04X)",
                  len, m_readPos, m_buffer.size(), m_opcode);
        size_t avail = m_buffer.size() - m_readPos;
        if (avail > 0 && dest) {
            std::memcpy(dest, &m_buffer[m_readPos], avail);
        }
        m_readPos = m_buffer.size();
        return;
    }
    if (dest && len > 0) {
        std::memcpy(dest, &m_buffer[m_readPos], len);
    }
    m_readPos += len;
}

// --- sf::Packet conversion ---

sf::Packet PacketBuffer::toSfPacket() const {
    sf::Packet sfPacket;
    // Write opcode as first 2 bytes, then the payload
    sfPacket << m_opcode;
    if (!m_buffer.empty()) {
        sfPacket.append(m_buffer.data(), m_buffer.size());
    }
    return sfPacket;
}

PacketBuffer PacketBuffer::fromSfPacket(sf::Packet& sfPacket) {
    uint16 opcode = 0;
    if (!(sfPacket >> opcode)) {
        LOG_ERROR("PacketBuffer::fromSfPacket - failed to read opcode");
        return PacketBuffer(0);
    }

    PacketBuffer pkt(opcode);

    // Remaining data in the sf::Packet is the payload.
    // sf::Packet tracks an internal read position after the >> extraction.
    // We can get the remaining data by computing offset from total size.
    const void* rawData = sfPacket.getData();
    size_t totalSize = sfPacket.getDataSize();

    // After extracting a uint16 (2 bytes), payload starts at offset 2
    constexpr size_t opcodeSize = sizeof(uint16);
    if (totalSize > opcodeSize) {
        size_t payloadSize = totalSize - opcodeSize;
        const uint8* payloadPtr = static_cast<const uint8*>(rawData) + opcodeSize;
        pkt.writeBytes(payloadPtr, payloadSize);
        pkt.resetRead();
    }

    return pkt;
}

// ============================================================================
//  GamePacket implementation
// ============================================================================

GamePacket::GamePacket(uint16 opcode)
    : m_buffer(opcode) {
}

GamePacket::GamePacket(uint16 opcode, PacketBuffer&& buffer)
    : m_buffer(std::move(buffer)) {
    m_buffer.setOpcode(opcode);
}
