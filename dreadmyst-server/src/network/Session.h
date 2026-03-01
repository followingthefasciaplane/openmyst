#pragma once
#include "core/Types.h"
#include "core/Log.h"
#include "network/GamePacket.h"

#include <SFML/Network.hpp>
#include <memory>
#include <string>
#include <queue>
#include <mutex>

// Forward declarations
class NetworkMgr;

// ============================================================================
//  Session - Represents a single client connection
// ============================================================================
// Original binary size: 0x150 bytes, reference counted via shared_ptr.
//
// Lifecycle:
//   1. AcceptConnections creates Session in Connected state with 5s auth timeout
//   2. Client sends CMSG_AUTHENTICATE -> state transitions to Authenticating
//   3. Server validates token -> state transitions to Authenticated
//   4. On disconnect or timeout -> state transitions to Disconnected
//
// Trust: sessions from localhost or whitelisted IPs are marked trusted,
//        enabling moderator commands (BAN/KICK/MUTE/WARN).
// ============================================================================

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(sf::TcpSocket&& socket);
    ~Session();

    // Non-copyable, non-movable (shared_ptr managed)
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;

    // --- Connection state ---
    SessionState getState() const { return m_state; }
    void         setState(SessionState state);
    bool         isConnected() const;
    bool         isAuthenticated() const;

    // --- Send / Receive ---
    // Send a game packet to this session's client.
    // Thread-safe: queues the packet for sending on the network thread.
    void send(const GamePacket& packet);

    // Receive a packet from the socket. Returns true if a packet was received.
    // Called by NetworkMgr during ProcessNetworkIO.
    bool receive(GamePacket& outPacket);

    // Flush the outgoing send queue to the socket.
    // Called by NetworkMgr during ProcessNetworkIO.
    void flushSendQueue();

    // --- Disconnect ---
    void disconnect();

    // --- Identity ---
    uint32      getAccountId()  const { return m_accountId; }
    void        setAccountId(uint32 id) { m_accountId = id; }

    Guid        getPlayerGuid() const { return m_playerGuid; }
    void        setPlayerGuid(Guid guid) { m_playerGuid = guid; }

    bool        isTrusted()     const { return m_trusted; }
    void        setTrusted(bool trusted) { m_trusted = trusted; }

    const std::string& getIpAddress() const { return m_ipAddress; }

    // --- Timeout ---
    // Auth timeout: if the session does not authenticate within this period
    // (default 5 seconds), the connection is dropped.
    float       getTimeoutRemaining() const { return m_timeoutRemaining; }
    void        setTimeoutRemaining(float seconds) { m_timeoutRemaining = seconds; }
    void        updateTimeout(float deltaSeconds);
    bool        hasTimedOut() const { return m_timedOut; }
    void        clearTimeout();

    // --- Socket access (for NetworkMgr) ---
    sf::TcpSocket& getSocket() { return m_socket; }

private:
    sf::TcpSocket   m_socket;
    SessionState    m_state           = SessionState::Disconnected;

    uint32          m_accountId       = 0;
    Guid            m_playerGuid      = 0;
    bool            m_trusted         = false;
    bool            m_timedOut        = false;
    float           m_timeoutRemaining = 0.0f;

    std::string     m_ipAddress;

    // Thread-safe outgoing packet queue
    std::queue<GamePacket> m_sendQueue;
    std::mutex             m_sendMutex;
};

using SessionPtr = std::shared_ptr<Session>;
