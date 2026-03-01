#pragma once

#include "SfSocket.h"
#include <memory>
#include <string>

// Connection - manages TCP connection to game server.
//
// Binary references:
//   Global singleton: DAT_00682884 at 0x00682884 (SfSocket* + TcpSocketEx* + refcount)
//   Singleton accessor: FUN_00404180 at 0x00404180
//   Connect method: FUN_00424ef0 at 0x00424ef0
//   Config read: [Tcp] section with Address, Port, Timeout
//
// Flow:
//   1. connect() reads Address/Port/Timeout from config.ini [Tcp]
//   2. Creates TcpSocketEx, calls sf::TcpSocket::connect()
//   3. Wraps in SfSocket for packet I/O
//   4. update() polls each frame via SfSocket::update()
//   5. popReceived() retrieves completed packets

class Connection
{
public:
    static Connection& instance();

    // Connect to game server using config.ini [Tcp] settings.
    // Binary: FUN_00424ef0
    // Returns true on success.
    bool connect();

    // Disconnect and clean up.
    void disconnect();

    // Poll for incoming packets and flush sends. Call each frame.
    // Returns false if disconnected.
    bool update();

    // Get received packets since last call.
    void popReceived(std::vector<ReceivedPacket>& output);

    // Send a packet with opcode + StlBuffer payload.
    void sendPacket(uint16_t opcode, const StlBuffer& payload);

    // Check connection state.
    bool isConnected() const;

    // Server address info (for display)
    const std::string& getAddress() const { return m_address; }
    uint16_t getPort() const { return m_port; }

private:
    Connection() = default;
    ~Connection() = default;

    std::shared_ptr<TcpSocketEx> m_tcpSocket;
    std::unique_ptr<SfSocket>    m_sfSocket;

    std::string m_address;
    uint16_t    m_port    = 16383;
    int         m_timeout = 5;
};

#define sConnection Connection::instance()
