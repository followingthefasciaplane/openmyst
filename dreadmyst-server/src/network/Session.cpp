#include "network/Session.h"

// ============================================================================
//  Session implementation
// ============================================================================

Session::Session(sf::TcpSocket&& socket)
    : m_socket(std::move(socket))
    , m_state(SessionState::Connected)
    , m_timeoutRemaining(static_cast<float>(AUTH_TIMEOUT_SECONDS)) {

    // Extract the remote IP address for logging and ban checks.
    // sf::TcpSocket::getRemoteAddress() returns the connected peer's address.
    auto remote = m_socket.getRemoteAddress();
    m_ipAddress = remote ? remote->toString() : "unknown";

    LOG_INFO("Session created from %s", m_ipAddress.c_str());
}

Session::~Session() {
    if (m_state != SessionState::Disconnected) {
        disconnect();
    }
    LOG_DEBUG("Session destroyed (account=%u, ip=%s)", m_accountId, m_ipAddress.c_str());
}

// --- Connection state ---

void Session::setState(SessionState state) {
    if (m_state == state) return;
    LOG_DEBUG("Session %s state change: %d -> %d",
              m_ipAddress.c_str(),
              static_cast<int>(m_state),
              static_cast<int>(state));
    m_state = state;
}

bool Session::isConnected() const {
    return m_state != SessionState::Disconnected;
}

bool Session::isAuthenticated() const {
    return m_state == SessionState::Authenticated;
}

// --- Send / Receive ---

void Session::send(const GamePacket& packet) {
    if (m_state == SessionState::Disconnected) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_sendMutex);
    m_sendQueue.push(packet);
}

bool Session::receive(GamePacket& outPacket) {
    if (m_state == SessionState::Disconnected) {
        return false;
    }

    sf::Packet sfPacket;
    sf::Socket::Status status = m_socket.receive(sfPacket);

    switch (status) {
        case sf::Socket::Status::Done: {
            // Successfully received a complete packet
            PacketBuffer pktBuf = PacketBuffer::fromSfPacket(sfPacket);
            uint16 opcode = pktBuf.getOpcode();

            if (!isValidOpcode(opcode)) {
                LOG_WARN("Session %s sent invalid opcode 0x%04X, disconnecting",
                         m_ipAddress.c_str(), opcode);
                disconnect();
                return false;
            }

            outPacket = GamePacket(opcode, std::move(pktBuf));
            return true;
        }

        case sf::Socket::Status::NotReady:
            // No data available (non-blocking socket), normal condition
            return false;

        case sf::Socket::Status::Partial:
            // Incomplete packet, SFML will buffer and complete on next receive
            return false;

        case sf::Socket::Status::Disconnected:
            LOG_INFO("Session %s disconnected by remote", m_ipAddress.c_str());
            disconnect();
            return false;

        case sf::Socket::Status::Error:
        default:
            LOG_ERROR("Session %s socket receive error", m_ipAddress.c_str());
            disconnect();
            return false;
    }
}

void Session::flushSendQueue() {
    if (m_state == SessionState::Disconnected) {
        return;
    }

    std::queue<GamePacket> toSend;
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        std::swap(toSend, m_sendQueue);
    }

    while (!toSend.empty()) {
        const GamePacket& pkt = toSend.front();
        sf::Packet sfPacket = pkt.toSfPacket();

        sf::Socket::Status status = m_socket.send(sfPacket);
        if (status != sf::Socket::Status::Done) {
            if (status == sf::Socket::Status::Disconnected) {
                LOG_INFO("Session %s disconnected during send", m_ipAddress.c_str());
            } else {
                LOG_ERROR("Session %s send error for opcode %s (0x%04X)",
                          m_ipAddress.c_str(),
                          opcodeToString(pkt.getOpcode()),
                          pkt.getOpcode());
            }
            disconnect();
            return;
        }

        LOG_DEBUG("Session %s -> %s (0x%04X, %zu bytes)",
                  m_ipAddress.c_str(),
                  opcodeToString(pkt.getOpcode()),
                  pkt.getOpcode(),
                  pkt.size());

        toSend.pop();
    }
}

// --- Disconnect ---

void Session::disconnect() {
    if (m_state == SessionState::Disconnected) {
        return;
    }

    LOG_INFO("Session %s disconnecting (account=%u, player=%u)",
             m_ipAddress.c_str(), m_accountId, m_playerGuid);

    m_state = SessionState::Disconnected;
    m_socket.disconnect();
}

// --- Timeout ---

void Session::updateTimeout(float deltaSeconds) {
    if (m_state == SessionState::Authenticated || m_timedOut) {
        return;  // No timeout once authenticated
    }

    m_timeoutRemaining -= deltaSeconds;
    if (m_timeoutRemaining <= 0.0f) {
        m_timedOut = true;
        LOG_WARN("Session %s auth timeout (account=%u)", m_ipAddress.c_str(), m_accountId);
    }
}

void Session::clearTimeout() {
    m_timedOut = false;
    m_timeoutRemaining = 0.0f;
}
