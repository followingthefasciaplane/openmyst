#include "network/NetworkMgr.h"
#include "packets/PacketHandler.h"
#include "core/Config.h"

#include <algorithm>
#include <chrono>

// ============================================================================
//  NetworkMgr implementation
// ============================================================================

NetworkMgr::NetworkMgr() {
    // Localhost and loopback are always whitelisted (trusted)
    m_whitelistedIPs.insert("127.0.0.1");
    m_whitelistedIPs.insert("::1");
    m_whitelistedIPs.insert("0.0.0.0");
}

NetworkMgr::~NetworkMgr() {
    shutdown();
}

// --- Initialization ---

bool NetworkMgr::initialize(uint16 port) {
    // Load limits from config
    auto& config = Config::instance();
    m_maxSessions = static_cast<uint32>(config.getInt("Network", "MaxSessions", 500));
    m_maxPlayers  = static_cast<uint32>(config.getInt("Network", "MaxPlayers", 200));

    // Load additional whitelisted IPs from config
    std::string extraWhitelist = config.getString("Network", "WhitelistedIPs", "");
    if (!extraWhitelist.empty()) {
        // Simple comma-separated parsing
        size_t start = 0;
        while (start < extraWhitelist.size()) {
            size_t end = extraWhitelist.find(',', start);
            if (end == std::string::npos) end = extraWhitelist.size();
            std::string ip = extraWhitelist.substr(start, end - start);
            // Trim whitespace
            while (!ip.empty() && ip.front() == ' ') ip.erase(ip.begin());
            while (!ip.empty() && ip.back() == ' ') ip.pop_back();
            if (!ip.empty()) {
                m_whitelistedIPs.insert(ip);
                LOG_INFO("NetworkMgr: whitelisted IP %s", ip.c_str());
            }
            start = end + 1;
        }
    }

    // Bind the TCP listener
    m_listener.setBlocking(false);
    sf::Socket::Status status = m_listener.listen(port);
    if (status != sf::Socket::Status::Done) {
        LOG_FATAL("NetworkMgr: failed to listen on port %u (status=%d)", port, static_cast<int>(status));
        return false;
    }

    m_listening = true;
    LOG_INFO("NetworkMgr: listening on port %u (maxSessions=%u, maxPlayers=%u)",
             port, m_maxSessions, m_maxPlayers);

    return true;
}

void NetworkMgr::shutdown() {
    if (!m_listening) return;

    LOG_INFO("NetworkMgr: shutting down...");

    // Disconnect all sessions
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        for (auto& session : m_sessions) {
            if (session && session->isConnected()) {
                session->disconnect();
            }
        }
        m_sessions.clear();
    }

    m_listener.close();
    m_listening = false;

    LOG_INFO("NetworkMgr: shutdown complete");
}

// --- AcceptConnections (0x00422310 reimplementation) ---

void NetworkMgr::acceptConnections() {
    if (!m_listening) return;

    // Accept up to MAX_ACCEPTS_PER_TICK connections per call to avoid
    // stalling the game loop under connection floods.
    for (int i = 0; i < MAX_ACCEPTS_PER_TICK; ++i) {
        if (!acceptOneConnection()) {
            break;  // No more pending connections
        }
    }
}

bool NetworkMgr::acceptOneConnection() {
    // Check session limit before accepting
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        if (m_sessions.size() >= m_maxSessions) {
            return false;
        }
    }

    // Create a new socket and attempt to accept a connection.
    // Original binary allocated a TcpSocketEx (0x30 bytes) before accept.
    sf::TcpSocket socket;
    sf::Socket::Status status = m_listener.accept(socket);

    if (status != sf::Socket::Status::Done) {
        // sf::Socket::NotReady means no pending connection (non-blocking listener)
        return false;
    }

    // Extract remote address before moving the socket
    auto remoteAddr = socket.getRemoteAddress();
    std::string remoteIP = remoteAddr ? remoteAddr->toString() : "unknown";
    uint16 remotePort = socket.getRemotePort();

    LOG_INFO("NetworkMgr: incoming connection from %s:%u", remoteIP.c_str(), remotePort);

    // --- IP ban check ---
    if (isIPBanned(remoteIP)) {
        LOG_WARN("NetworkMgr: rejected banned IP %s", remoteIP.c_str());
        socket.disconnect();
        return true;  // We did accept (and reject) a connection
    }

    // --- Rate limit check ---
    if (isRateLimited(remoteIP)) {
        LOG_WARN("NetworkMgr: rate-limited IP %s, rejecting", remoteIP.c_str());
        socket.disconnect();
        return true;
    }

    recordConnectionAttempt(remoteIP);

    // Set non-blocking mode for the accepted socket.
    // The original binary sets non-blocking immediately after accept.
    socket.setBlocking(false);

    // --- Create the session ---
    auto session = std::make_shared<Session>(std::move(socket));

    // Set 5-second auth timeout (AUTH_TIMEOUT_SECONDS from Types.h)
    session->setTimeoutRemaining(static_cast<float>(AUTH_TIMEOUT_SECONDS));

    // Trust check: localhost and whitelisted IPs get trusted status,
    // which enables moderator commands.
    if (isIPWhitelisted(remoteIP)) {
        session->setTrusted(true);
        LOG_INFO("NetworkMgr: session from %s marked as trusted", remoteIP.c_str());
    }

    // Add to active sessions
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        m_sessions.push_back(session);
    }

    LOG_INFO("NetworkMgr: session created for %s (total sessions: %zu)",
             remoteIP.c_str(), getSessionCount());

    return true;
}

// --- ProcessNetworkIO (0x00422c90 reimplementation) ---

void NetworkMgr::processNetworkIO(float deltaSeconds) {
    if (!m_listening) return;

    // Build a snapshot of current sessions to avoid holding the lock during I/O
    std::vector<SessionPtr> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        snapshot = m_sessions;
    }

    for (auto& session : snapshot) {
        if (!session || !session->isConnected()) {
            continue;
        }

        // --- Update auth timeout for unauthenticated sessions ---
        if (!session->isAuthenticated()) {
            session->updateTimeout(deltaSeconds);
            if (session->hasTimedOut()) {
                LOG_WARN("NetworkMgr: auth timeout for %s, disconnecting",
                         session->getIpAddress().c_str());
                session->disconnect();
                continue;
            }
        }

        // --- Receive and dispatch packets ---
        // Process multiple packets per session per tick (the client may batch)
        GamePacket packet;
        int packetsThisTick = 0;
        constexpr int MAX_PACKETS_PER_SESSION_PER_TICK = 50;

        while (session->isConnected() &&
               packetsThisTick < MAX_PACKETS_PER_SESSION_PER_TICK &&
               session->receive(packet)) {

            LOG_DEBUG("NetworkMgr: %s <- %s (0x%04X, %zu bytes)",
                      session->getIpAddress().c_str(),
                      opcodeToString(packet.getOpcode()),
                      packet.getOpcode(),
                      packet.size());

            dispatchPacket(session, packet);
            ++packetsThisTick;
        }

        // --- Flush outgoing send queue ---
        if (session->isConnected()) {
            session->flushSendQueue();
        }
    }

    // Periodically prune rate limit entries
    m_rateLimitPruneAccum += deltaSeconds;
    if (m_rateLimitPruneAccum >= static_cast<float>(RATE_LIMIT_PRUNE_INTERVAL)) {
        m_rateLimitPruneAccum = 0.0f;
        pruneRateLimitEntries();
    }

    // Remove disconnected sessions
    pruneDeadSessions();
}

// --- Packet handler registration ---

void NetworkMgr::registerHandler(uint16 opcode, OpcodeHandler handler) {
    if (m_handlers.count(opcode)) {
        LOG_WARN("NetworkMgr: overwriting handler for opcode %s (0x%04X)",
                 opcodeToString(opcode), opcode);
    }
    m_handlers[opcode] = std::move(handler);
    LOG_DEBUG("NetworkMgr: registered handler for %s (0x%04X)",
              opcodeToString(opcode), opcode);
}

void NetworkMgr::dispatchPacket(SessionPtr session, GamePacket& packet) {
    uint16 opcode = packet.getOpcode();

    auto it = m_handlers.find(opcode);
    if (it != m_handlers.end()) {
        // Reset read position so the handler reads from the start of payload
        packet.resetRead();
        it->second(session, packet);
    } else {
        // Fallback to PacketHandler namespace dispatch (covers all game opcodes)
        packet.resetRead();
        PacketHandler::dispatch(session.get(), opcode, packet);
    }
}

// --- Session management ---

SessionPtr NetworkMgr::findSessionByAccountId(uint32 accountId) const {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    for (const auto& session : m_sessions) {
        if (session && session->isConnected() && session->getAccountId() == accountId) {
            return session;
        }
    }
    return nullptr;
}

SessionPtr NetworkMgr::findSessionByPlayerGuid(Guid guid) const {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    for (const auto& session : m_sessions) {
        if (session && session->isConnected() && session->getPlayerGuid() == guid) {
            return session;
        }
    }
    return nullptr;
}

void NetworkMgr::broadcastPacket(const GamePacket& packet) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    for (auto& session : m_sessions) {
        if (session && session->isAuthenticated()) {
            session->send(packet);
        }
    }
}

void NetworkMgr::broadcastPacketExcept(const GamePacket& packet, SessionPtr excluded) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    for (auto& session : m_sessions) {
        if (session && session->isAuthenticated() && session != excluded) {
            session->send(packet);
        }
    }
}

void NetworkMgr::disconnectSession(SessionPtr session) {
    if (session) {
        session->disconnect();
    }
}

size_t NetworkMgr::getSessionCount() const {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    return m_sessions.size();
}

size_t NetworkMgr::getAuthenticatedCount() const {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    size_t count = 0;
    for (const auto& session : m_sessions) {
        if (session && session->isAuthenticated()) {
            ++count;
        }
    }
    return count;
}

// --- Dead session pruning ---

void NetworkMgr::pruneDeadSessions() {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    auto it = std::remove_if(m_sessions.begin(), m_sessions.end(),
        [](const SessionPtr& s) {
            return !s || !s->isConnected();
        });

    size_t removed = std::distance(it, m_sessions.end());
    if (removed > 0) {
        m_sessions.erase(it, m_sessions.end());
        LOG_DEBUG("NetworkMgr: pruned %zu dead sessions (remaining: %zu)",
                  removed, m_sessions.size());
    }
}

// --- IP management ---

void NetworkMgr::addBannedIP(const std::string& ip) {
    std::lock_guard<std::mutex> lock(m_bannedIPsMutex);
    m_bannedIPs.insert(ip);
    LOG_INFO("NetworkMgr: banned IP %s", ip.c_str());
}

void NetworkMgr::removeBannedIP(const std::string& ip) {
    std::lock_guard<std::mutex> lock(m_bannedIPsMutex);
    m_bannedIPs.erase(ip);
    LOG_INFO("NetworkMgr: unbanned IP %s", ip.c_str());
}

bool NetworkMgr::isIPBanned(const std::string& ip) const {
    std::lock_guard<std::mutex> lock(m_bannedIPsMutex);
    return m_bannedIPs.count(ip) > 0;
}

void NetworkMgr::addWhitelistedIP(const std::string& ip) {
    m_whitelistedIPs.insert(ip);
    LOG_INFO("NetworkMgr: whitelisted IP %s", ip.c_str());
}

bool NetworkMgr::isIPWhitelisted(const std::string& ip) const {
    return m_whitelistedIPs.count(ip) > 0;
}

// --- Rate limiting ---

bool NetworkMgr::isRateLimited(const std::string& ip) {
    // Whitelisted IPs are never rate-limited
    if (isIPWhitelisted(ip)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_rateLimitMutex);
    auto it = m_rateLimitMap.find(ip);
    if (it == m_rateLimitMap.end()) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - it->second.firstAttempt).count();

    // If outside the window, this IP is not currently rate-limited
    if (elapsed >= RATE_LIMIT_WINDOW_SECONDS) {
        m_rateLimitMap.erase(it);
        return false;
    }

    return it->second.count >= RATE_LIMIT_MAX_CONNECTIONS;
}

void NetworkMgr::recordConnectionAttempt(const std::string& ip) {
    if (isIPWhitelisted(ip)) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_rateLimitMutex);
    auto now = std::chrono::steady_clock::now();

    auto it = m_rateLimitMap.find(ip);
    if (it == m_rateLimitMap.end()) {
        m_rateLimitMap[ip] = { 1, now };
    } else {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.firstAttempt).count();
        if (elapsed >= RATE_LIMIT_WINDOW_SECONDS) {
            // Reset window
            it->second = { 1, now };
        } else {
            it->second.count++;
        }
    }
}

void NetworkMgr::pruneRateLimitEntries() {
    std::lock_guard<std::mutex> lock(m_rateLimitMutex);
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_rateLimitMap.begin(); it != m_rateLimitMap.end(); ) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.firstAttempt).count();
        if (elapsed >= RATE_LIMIT_WINDOW_SECONDS) {
            it = m_rateLimitMap.erase(it);
        } else {
            ++it;
        }
    }
}
