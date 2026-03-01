#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

// Binary serialization buffer used for all packet I/O.
// Verified layout from binary decompilation (Dreadmyst.exe r1189):
//
//   offset 0x00: uint32_t m_readPos      - Current read position
//   offset 0x04: uint32_t m_writePos     - Current write position
//   offset 0x08: std::vector<uint8_t> m_data  - Buffer storage (begin/end/capacity)
//   offset 0x14: bool m_strict           - If true, throw on underflow; if false, return 0
//
// String format: null-terminated (NOT length-prefixed).
//   readString reads byte-by-byte until '\0'.
//   writeString writes bytes + '\0'.
//
// Integer format: little-endian (x86 native).

class StlBuffer
{
public:
    StlBuffer();
    explicit StlBuffer(bool strict);
    StlBuffer(const uint8_t* data, size_t size, bool strict = true);
    ~StlBuffer() = default;

    StlBuffer(const StlBuffer&) = default;
    StlBuffer& operator=(const StlBuffer&) = default;
    StlBuffer(StlBuffer&&) noexcept = default;
    StlBuffer& operator=(StlBuffer&&) noexcept = default;

    // --- Read methods (advance m_readPos) ---
    uint8_t     readUint8();
    uint16_t    readUint16();
    uint32_t    readUint32();
    int8_t      readInt8();
    int16_t     readInt16();
    int32_t     readInt32();
    float       readFloat();
    bool        readBool();
    std::string readString();
    std::string readLenString();  // Length-prefixed (uint16 len + data)
    void        readBytes(uint8_t* dest, size_t len);
    uint64_t    readUint64();

    // --- Write methods (append to buffer) ---
    void writeUint8(uint8_t value);
    void writeUint16(uint16_t value);
    void writeUint32(uint32_t value);
    void writeInt8(int8_t value);
    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void writeFloat(float value);
    void writeBool(bool value);
    void writeString(const std::string& value);
    void writeLenString(const std::string& value);  // Length-prefixed (uint16 len + data)
    void writeBytes(const uint8_t* data, size_t len);
    void writeUint64(uint64_t value);

    // --- Buffer access ---
    const uint8_t* data() const { return m_data.data(); }
    size_t size() const { return m_data.size(); }
    size_t readPos() const { return m_readPos; }
    size_t remaining() const;
    bool empty() const { return m_data.empty(); }

    void resetRead() { m_readPos = 0; }
    void clear();

    // Raw data access for network send
    const std::vector<uint8_t>& buffer() const { return m_data; }

    // Erase bytes from the front of the buffer (used after reading opcode)
    void eraseFront(size_t bytes);

    // Stream extraction operator (reads value and advances position)
    template<typename T>
    StlBuffer& operator>>(T& value)
    {
        ensureRead(sizeof(T));
        memcpy(&value, m_data.data() + m_readPos, sizeof(T));
        m_readPos += sizeof(T);
        return *this;
    }

private:
    bool canRead(size_t bytes) const;
    void ensureRead(size_t bytes);

    uint32_t             m_readPos  = 0;
    uint32_t             m_writePos = 0;
    std::vector<uint8_t> m_data;
    bool                 m_strict   = true;
};
