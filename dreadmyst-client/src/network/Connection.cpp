#include "Connection.h"
#include "Config.h"
#include "core/Log.h"
#include "StlBuffer.h"

#include <SFML/Network.hpp>

// Connection implementation from binary decompilation.
//
// Binary: FUN_00424ef0 (connect), FUN_00404180 (singleton accessor)
// Config: [Tcp] Address, Port, Timeout
// The binary reads from INI section at 0x0062d160 with keys at 0x0062d158/0x0062d164/0x0062d16c.

Connection& Connection::instance()
{
    static Connection inst;
    return inst;
}

// Binary: FUN_00424ef0
bool Connection::connect()
{
    // Disconnect existing connection if any
    disconnect();

    // Read config values from [Tcp] section
    m_address = sConfig->getString("Tcp", "Address", "127.0.0.1");
    m_port    = static_cast<uint16_t>(sConfig->getInt("Tcp", "Port", 16383));
    m_timeout = sConfig->getInt("Tcp", "Timeout", 5);

    // Strip quotes from address if present (config.ini has Address="...")
    if (m_address.size() >= 2 && m_address.front() == '"' && m_address.back() == '"')
    {
        m_address = m_address.substr(1, m_address.size() - 2);
    }

    LOG_INFO("Connecting to %s:%u (timeout=%ds)", m_address.c_str(), m_port, m_timeout);

    // Create the TCP socket
    m_tcpSocket = std::make_shared<TcpSocketEx>();

    // Binary: sets blocking for connect, then non-blocking after
    m_tcpSocket->setBlocking(true);

    // Resolve address and connect
    auto addr = sf::IpAddress::resolve(m_address);
    if (!addr.has_value())
    {
        LOG_ERROR("Failed to resolve address: %s", m_address.c_str());
        m_tcpSocket.reset();
        return false;
    }

    auto status = m_tcpSocket->connect(
        addr.value(),
        m_port,
        sf::seconds(static_cast<float>(m_timeout))
    );

    if (status != sf::Socket::Status::Done)
    {
        LOG_ERROR("Failed to connect to %s:%u (status=%d)",
                  m_address.c_str(), m_port, static_cast<int>(status));
        m_tcpSocket.reset();
        return false;
    }

    // Binary: set non-blocking after successful connect
    m_tcpSocket->setBlocking(false);

    // Wrap in SfSocket for packet I/O
    m_sfSocket = std::make_unique<SfSocket>(m_tcpSocket);

    LOG_INFO("Connected to %s:%u", m_address.c_str(), m_port);
    return true;
}

void Connection::disconnect()
{
    if (m_sfSocket)
    {
        m_sfSocket->cancel();
        m_sfSocket.reset();
    }
    m_tcpSocket.reset();
}

bool Connection::update()
{
    if (!m_sfSocket)
        return false;

    return m_sfSocket->update();
}

void Connection::popReceived(std::vector<ReceivedPacket>& output)
{
    if (m_sfSocket)
        m_sfSocket->popReceived(output);
}

void Connection::sendPacket(uint16_t opcode, const StlBuffer& payload)
{
    if (m_sfSocket)
        m_sfSocket->sendPacket(opcode, payload);
}

bool Connection::isConnected() const
{
    return m_sfSocket && m_sfSocket->connected();
}
