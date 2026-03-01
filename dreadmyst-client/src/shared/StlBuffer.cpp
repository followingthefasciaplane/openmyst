#include "StlBuffer.h"

#include <cstring>
#include <stdexcept>

// StlBuffer implementation verified against binary decompilation.
// Layout confirmed: m_readPos(0x00), m_writePos(0x04), m_data(0x08), m_strict(0x14).
// String format: null-terminated (NOT length-prefixed).
// Integer format: little-endian (x86 native, memcpy safe on LE platforms).

StlBuffer::StlBuffer()
    : m_readPos(0), m_writePos(0), m_strict(true)
{
}

StlBuffer::StlBuffer(bool strict)
    : m_readPos(0), m_writePos(0), m_strict(strict)
{
}

StlBuffer::StlBuffer(const uint8_t* data, size_t size, bool strict)
    : m_readPos(0), m_writePos(static_cast<uint32_t>(size)),
      m_data(data, data + size), m_strict(strict)
{
}

bool StlBuffer::canRead(size_t bytes) const
{
    return (m_readPos + bytes) <= m_data.size();
}

void StlBuffer::ensureRead(size_t bytes)
{
    if (!canRead(bytes))
    {
        if (m_strict)
            throw std::runtime_error("StlBuffer: read underflow");
    }
}

size_t StlBuffer::remaining() const
{
    if (m_readPos >= m_data.size())
        return 0;
    return m_data.size() - m_readPos;
}

void StlBuffer::clear()
{
    m_data.clear();
    m_readPos = 0;
    m_writePos = 0;
}

void StlBuffer::eraseFront(size_t bytes)
{
    if (bytes >= m_data.size())
    {
        clear();
        return;
    }
    m_data.erase(m_data.begin(), m_data.begin() + bytes);
    if (m_readPos >= bytes)
        m_readPos -= static_cast<uint32_t>(bytes);
    else
        m_readPos = 0;
}

// --- Read methods ---

uint8_t StlBuffer::readUint8()
{
    ensureRead(1);
    if (!canRead(1)) return 0;
    uint8_t val = m_data[m_readPos];
    m_readPos += 1;
    return val;
}

uint16_t StlBuffer::readUint16()
{
    ensureRead(2);
    if (!canRead(2)) return 0;
    uint16_t val;
    std::memcpy(&val, m_data.data() + m_readPos, 2);
    m_readPos += 2;
    return val;
}

uint32_t StlBuffer::readUint32()
{
    ensureRead(4);
    if (!canRead(4)) return 0;
    uint32_t val;
    std::memcpy(&val, m_data.data() + m_readPos, 4);
    m_readPos += 4;
    return val;
}

int8_t StlBuffer::readInt8()
{
    return static_cast<int8_t>(readUint8());
}

int16_t StlBuffer::readInt16()
{
    return static_cast<int16_t>(readUint16());
}

int32_t StlBuffer::readInt32()
{
    return static_cast<int32_t>(readUint32());
}

float StlBuffer::readFloat()
{
    ensureRead(4);
    if (!canRead(4)) return 0.0f;
    float val;
    std::memcpy(&val, m_data.data() + m_readPos, 4);
    m_readPos += 4;
    return val;
}

bool StlBuffer::readBool()
{
    return readUint8() != 0;
}

std::string StlBuffer::readString()
{
    // Binary format: null-terminated string, read byte by byte until '\0'
    std::string result;
    while (canRead(1))
    {
        uint8_t ch = m_data[m_readPos++];
        if (ch == 0)
            break;
        result += static_cast<char>(ch);
    }
    return result;
}

void StlBuffer::readBytes(uint8_t* dest, size_t len)
{
    ensureRead(len);
    if (!canRead(len)) return;
    std::memcpy(dest, m_data.data() + m_readPos, len);
    m_readPos += static_cast<uint32_t>(len);
}

uint64_t StlBuffer::readUint64()
{
    ensureRead(8);
    if (!canRead(8)) return 0;
    uint64_t val;
    std::memcpy(&val, m_data.data() + m_readPos, 8);
    m_readPos += 8;
    return val;
}

// --- Write methods ---

void StlBuffer::writeUint8(uint8_t value)
{
    m_data.push_back(value);
    m_writePos += 1;
}

void StlBuffer::writeUint16(uint16_t value)
{
    const auto* p = reinterpret_cast<const uint8_t*>(&value);
    m_data.insert(m_data.end(), p, p + 2);
    m_writePos += 2;
}

void StlBuffer::writeUint32(uint32_t value)
{
    const auto* p = reinterpret_cast<const uint8_t*>(&value);
    m_data.insert(m_data.end(), p, p + 4);
    m_writePos += 4;
}

void StlBuffer::writeInt8(int8_t value)
{
    writeUint8(static_cast<uint8_t>(value));
}

void StlBuffer::writeInt16(int16_t value)
{
    writeUint16(static_cast<uint16_t>(value));
}

void StlBuffer::writeInt32(int32_t value)
{
    writeUint32(static_cast<uint32_t>(value));
}

void StlBuffer::writeFloat(float value)
{
    const auto* p = reinterpret_cast<const uint8_t*>(&value);
    m_data.insert(m_data.end(), p, p + 4);
    m_writePos += 4;
}

void StlBuffer::writeBool(bool value)
{
    writeUint8(value ? 1 : 0);
}

void StlBuffer::writeString(const std::string& value)
{
    // Binary format: write bytes + null terminator
    m_data.insert(m_data.end(), value.begin(), value.end());
    m_data.push_back(0);
    m_writePos += static_cast<uint32_t>(value.size() + 1);
}

void StlBuffer::writeBytes(const uint8_t* data, size_t len)
{
    m_data.insert(m_data.end(), data, data + len);
    m_writePos += static_cast<uint32_t>(len);
}

void StlBuffer::writeLenString(const std::string& value)
{
    // Server-compatible format: uint16 length + raw data (no null terminator)
    uint16_t len = static_cast<uint16_t>(value.size());
    writeUint16(len);
    if (len > 0)
    {
        m_data.insert(m_data.end(), value.begin(), value.end());
        m_writePos += static_cast<uint32_t>(len);
    }
}

std::string StlBuffer::readLenString()
{
    // Server-compatible format: uint16 length + raw data
    uint16_t len = readUint16();
    if (len == 0)
        return {};
    if (!canRead(len))
    {
        if (m_strict)
            throw std::runtime_error("StlBuffer: readLenString underflow");
        return {};
    }
    std::string result(reinterpret_cast<const char*>(m_data.data() + m_readPos), len);
    m_readPos += len;
    return result;
}

void StlBuffer::writeUint64(uint64_t value)
{
    const auto* p = reinterpret_cast<const uint8_t*>(&value);
    m_data.insert(m_data.end(), p, p + 8);
    m_writePos += 8;
}
