#pragma once

#include <SFML/Network.hpp>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>

class StlBuffer;

// TcpSocketEx - Extended SFML TCP socket with shared_ptr management
class TcpSocketEx : public sf::TcpSocket
{
public:
    TcpSocketEx() = default;
    ~TcpSocketEx() = default;
};

// Received packet: opcode extracted from sf::Packet, payload as raw LE bytes
struct ReceivedPacket
{
    uint16_t opcode = 0;
    std::vector<uint8_t> payload;
};

// SFML TCP socket wrapper with non-blocking polling.
// Binary: SfSocket at RTTI .?AVSfSocket@@, object size 0x90.
//
// Wire format (compatible with reimplemented server):
//   sf::Packet framing (4-byte BE size header, handled by SFML)
//   Inside: [uint16 opcode (SFML format)][payload bytes (LE)]
//
// Usage:
//   m_sfSocket = make_unique<SfSocket>(m_socket);
//   m_sfSocket->update();           // poll each frame
//   m_sfSocket->popReceived(out);   // get completed packets
//   m_sfSocket->sendPacket(opcode, payload);
class SfSocket
{
public:
    explicit SfSocket(std::shared_ptr<TcpSocketEx> socket);
    ~SfSocket();

    // Called every frame - receives data, assembles packets, flushes sends.
    // Returns false if disconnected.
    // Binary: FUN_005557a0
    bool update();

    // Cancel pending operations and disconnect
    // Binary: FUN_00555a50
    void cancel();

    // Send a packet: opcode + payload from StlBuffer
    // Binary: FUN_00424e60
    void sendPacket(uint16_t opcode, const StlBuffer& payload);

    // Send raw sf::Packet (for packets built externally)
    void sendRaw(sf::Packet& packet);

    // Pop all received packets into output vector
    void popReceived(std::vector<ReceivedPacket>& output);

    // Check if still connected
    bool connected() const;

private:
    std::shared_ptr<TcpSocketEx> m_socket;
    std::atomic<bool> m_connected{false};

    std::mutex m_sendMutex;
    std::vector<ReceivedPacket> m_receivedPackets;

    // Bandwidth stats (from binary at offsets 0x10-0x34)
    uint32_t m_bytesSentPerSec   = 0;
    uint32_t m_bytesRecvPerSec   = 0;
    uint32_t m_totalBytesSent    = 0;
    uint32_t m_totalBytesRecv    = 0;
    uint32_t m_totalPacketsSent  = 0;
    uint32_t m_totalPacketsRecv  = 0;
    int64_t  m_lastStatTime      = 0;
};
