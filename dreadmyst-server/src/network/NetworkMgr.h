#pragma once
#include "core/Types.h"
#include "core/Log.h"
#include "network/Session.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"

#include <SFML/Network.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <mutex>
#include <memory>
#include <chrono>

// ============================================================================
//  NetworkMgr - TCP listener and session management
// ============================================================================
// Reimplementation of the original network manager.
//
// AcceptConnections (0x00422310):
//   - Accepts up to MAX_ACCEPTS_PER_TICK new TCP connections per tick
//   - Creates a TcpSocket (original TcpSocketEx was 0x30 bytes), sets non-blocking
//   - Checks IP bans and rate limits before creating a Session
//   - Sets 5-second auth timeout on new sessions
//   - If remote address is localhost or whitelisted, session.trusted = true
//
// ProcessNetworkIO (0x00422c90):
//   - Iterates all active sessions
//   - Receives packets and dispatches to registered handlers
//   - Manages pending auth queue (disconnects timed-out sessions)
//   - Flushes outgoing send queues
// ============================================================================

// Packet handler callback: receives the session and the parsed packet
using OpcodeHandler = std::function<void(SessionPtr, GamePacket&)>;

class NetworkMgr {
public:
    NetworkMgr();
    ~NetworkMgr();

    // Non-copyable
    NetworkMgr(const NetworkMgr&) = delete;
    NetworkMgr& operator=(const NetworkMgr&) = delete;

    // --- Initialization ---
    // Bind the TCP listener to the configured port. Returns false on failure.
    bool initialize(uint16 port = DEFAULT_TCP_PORT);
    void shutdown();

    // --- Per-tick processing (called from Server main loop) ---
    // Accept new incoming connections (up to MAX_ACCEPTS_PER_TICK per call).
    void acceptConnections();

    // Receive packets, dispatch handlers, flush send queues, prune dead sessions.
    void processNetworkIO(float deltaSeconds);

    // --- Packet handler registration ---
    // Register a handler for a specific client opcode.
    void registerHandler(uint16 opcode, OpcodeHandler handler);

    // --- Session management ---
    SessionPtr findSessionByAccountId(uint32 accountId) const;
    SessionPtr findSessionByPlayerGuid(Guid guid) const;

    // Send a packet to all authenticated sessions.
    void broadcastPacket(const GamePacket& packet);

    // Send a packet to all authenticated sessions except the excluded one.
    void broadcastPacketExcept(const GamePacket& packet, SessionPtr excluded);

    // Forcefully disconnect a session.
    void disconnectSession(SessionPtr session);

    // --- Statistics ---
    size_t getSessionCount() const;
    size_t getAuthenticatedCount() const;

    // --- IP management ---
    void addBannedIP(const std::string& ip);
    void removeBannedIP(const std::string& ip);
    bool isIPBanned(const std::string& ip) const;

    void addWhitelistedIP(const std::string& ip);
    bool isIPWhitelisted(const std::string& ip) const;

    // --- Limits ---
    void   setMaxSessions(uint32 max) { m_maxSessions = max; }
    uint32 getMaxSessions() const { return m_maxSessions; }
    void   setMaxPlayers(uint32 max) { m_maxPlayers = max; }
    uint32 getMaxPlayers() const { return m_maxPlayers; }

private:
    // Attempt to accept a single connection. Returns true if a connection was accepted.
    bool acceptOneConnection();

    // Check if a new connection from this IP should be rate-limited.
    bool isRateLimited(const std::string& ip);

    // Record a connection attempt from this IP for rate-limiting purposes.
    void recordConnectionAttempt(const std::string& ip);

    // Clean up expired rate limit entries.
    void pruneRateLimitEntries();

    // Remove disconnected sessions from the active list.
    void pruneDeadSessions();

    // Dispatch a received packet to its registered handler.
    void dispatchPacket(SessionPtr session, GamePacket& packet);

    // TCP listener
    sf::TcpListener m_listener;
    bool            m_listening = false;

    // Active sessions
    std::vector<SessionPtr> m_sessions;
    mutable std::mutex      m_sessionsMutex;

    // Packet handlers keyed by opcode
    std::unordered_map<uint16, OpcodeHandler> m_handlers;

    // IP ban list
    std::unordered_set<std::string> m_bannedIPs;
    mutable std::mutex              m_bannedIPsMutex;

    // IP whitelist (trusted IPs, e.g. localhost)
    std::unordered_set<std::string> m_whitelistedIPs;

    // Rate limiting: tracks recent connection attempts per IP
    struct RateLimitEntry {
        int      count = 0;
        std::chrono::steady_clock::time_point firstAttempt;
    };
    std::unordered_map<std::string, RateLimitEntry> m_rateLimitMap;
    std::mutex                                       m_rateLimitMutex;

    // Rate limit settings
    static constexpr int    RATE_LIMIT_MAX_CONNECTIONS = 10;    // max connections per window
    static constexpr int    RATE_LIMIT_WINDOW_SECONDS  = 60;    // window duration
    static constexpr int    RATE_LIMIT_PRUNE_INTERVAL  = 300;   // prune stale entries every 5 min

    // Connection limits
    uint32 m_maxSessions = 500;
    uint32 m_maxPlayers  = 200;

    // Prune timer
    float m_rateLimitPruneAccum = 0.0f;
};
