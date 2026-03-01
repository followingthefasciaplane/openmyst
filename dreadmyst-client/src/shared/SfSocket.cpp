#include "SfSocket.h"
#include "StlBuffer.h"

#include <cstring>
#include <ctime>

// SfSocket implementation -- polling model from binary decompilation.
//
// Binary references:
//   Constructor:  FUN_00555510 at 0x00555510
//   Update/Poll:  FUN_005557a0 at 0x005557a0
//   SendPacket:   FUN_00424e60 at 0x00424e60
//   Disconnect:   FUN_00555a50 at 0x00555a50
//   Destructor:   FUN_00555720 at 0x00555720
//
// Wire format: sf::Packet (SFML handles framing).
//   Inside packet: [uint16 opcode via SFML <<][payload bytes in LE]
//   This is compatible with the reimplemented server which uses
//   PacketBuffer::toSfPacket() / fromSfPacket().

SfSocket::SfSocket(std::shared_ptr<TcpSocketEx> socket)
    : m_socket(std::move(socket))
{
    m_connected = true;

    // Binary: sets TCP_NODELAY to disable Nagle's algorithm
    m_socket->setBlocking(false);
}

SfSocket::~SfSocket()
{
    cancel();
}

// Binary: FUN_005557a0
// Non-blocking poll: receive all available packets, flush pending sends.
bool SfSocket::update()
{
    if (!m_connected.load())
        return false;

    // Receive loop: pull complete sf::Packets from the socket
    sf::Packet sfPacket;
    while (true)
    {
        auto status = m_socket->receive(sfPacket);

        if (status == sf::Socket::Status::Done)
        {
            // Extract opcode (first uint16 via SFML deserialization)
            uint16_t opcode = 0;
            if (!(sfPacket >> opcode))
            {
                // Malformed packet -- no opcode
                continue;
            }

            // Extract remaining payload bytes
            ReceivedPacket pkt;
            pkt.opcode = opcode;

            const void* rawData = sfPacket.getData();
            size_t totalSize = sfPacket.getDataSize();
            constexpr size_t opcodeSize = sizeof(uint16_t);

            if (totalSize > opcodeSize)
            {
                size_t payloadSize = totalSize - opcodeSize;
                const uint8_t* payloadPtr =
                    static_cast<const uint8_t*>(rawData) + opcodeSize;
                pkt.payload.assign(payloadPtr, payloadPtr + payloadSize);
            }

            m_receivedPackets.push_back(std::move(pkt));
            m_totalBytesRecv += static_cast<uint32_t>(totalSize);
            m_totalPacketsRecv++;

            sfPacket.clear();
        }
        else if (status == sf::Socket::Status::NotReady)
        {
            break;  // No more data available
        }
        else
        {
            // Disconnected or error
            cancel();
            return false;
        }
    }

    // Binary: bandwidth stats update every 3 seconds
    int64_t now = static_cast<int64_t>(std::time(nullptr));
    if (now > m_lastStatTime + 2)
    {
        m_bytesSentPerSec  = m_totalBytesSent / 3;
        m_bytesRecvPerSec  = m_totalBytesRecv / 3;
        m_totalBytesSent   = 0;
        m_totalBytesRecv   = 0;
        m_totalPacketsSent = 0;
        m_totalPacketsRecv = 0;
        m_lastStatTime     = now;
    }

    return true;
}

// Binary: FUN_00555a50
void SfSocket::cancel()
{
    if (m_connected.load())
    {
        m_connected = false;
        m_socket->disconnect();

        // Binary: reset all stats on disconnect
        m_totalBytesSent   = 0;
        m_totalBytesRecv   = 0;
        m_totalPacketsSent = 0;
        m_totalPacketsRecv = 0;
        m_bytesSentPerSec  = 0;
        m_bytesRecvPerSec  = 0;
        m_lastStatTime     = 0;
    }
}

// Binary: FUN_00424e60
// Builds sf::Packet from opcode + StlBuffer payload and sends.
void SfSocket::sendPacket(uint16_t opcode, const StlBuffer& payload)
{
    if (!m_connected.load())
        return;

    sf::Packet sfPacket;
    sfPacket << opcode;

    if (payload.size() > 0)
    {
        sfPacket.append(payload.data(), payload.size());
    }

    auto status = m_socket->send(sfPacket);
    if (status == sf::Socket::Status::Disconnected ||
        status == sf::Socket::Status::Error)
    {
        cancel();
        return;
    }

    m_totalBytesSent += static_cast<uint32_t>(sfPacket.getDataSize());
    m_totalPacketsSent++;
}

void SfSocket::sendRaw(sf::Packet& packet)
{
    if (!m_connected.load())
        return;

    auto status = m_socket->send(packet);
    if (status == sf::Socket::Status::Disconnected ||
        status == sf::Socket::Status::Error)
    {
        cancel();
    }
}

void SfSocket::popReceived(std::vector<ReceivedPacket>& output)
{
    output = std::move(m_receivedPackets);
    m_receivedPackets.clear();
}

bool SfSocket::connected() const
{
    return m_connected.load();
}
