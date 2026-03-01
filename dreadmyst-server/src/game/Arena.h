#pragma once
#include "core/Types.h"
#include "game/templates/MapTemplate.h"
#include <vector>
#include <memory>
#include <mutex>

class ServerPlayer;

enum class ArenaState : uint8 {
    Waiting     = 0,
    Countdown60 = 1,
    Countdown45 = 2,
    Countdown30 = 3,
    Countdown15 = 4,
    InProgress  = 5,
    Ended       = 6
};

struct ArenaTeam {
    std::vector<Guid> playerGuids;
    int32 totalRating = 0;
};

class Arena {
public:
    Arena(const ArenaTemplate& tmpl, uint32 instanceId);

    void addTeam1Player(Guid guid);
    void addTeam2Player(Guid guid);

    void start();
    void update(int32 deltaMs);
    void endMatch(int winningTeam); // 0=draw, 1=team1, 2=team2

    ArenaState getState() const { return m_state; }
    uint32 getInstanceId() const { return m_instanceId; }

    void onPlayerDeath(Guid guid);
    void onPlayerDisconnect(Guid guid);

private:
    void broadcastMessage(const std::string& msg);
    void teleportPlayersToStartPositions();
    void awardRating(int winningTeam);
    bool isTeamEliminated(int team) const;

    ArenaTemplate m_template;
    uint32        m_instanceId = 0;
    ArenaState    m_state      = ArenaState::Waiting;
    int32         m_countdownTimer = 0;
    ArenaTeam     m_team1;
    ArenaTeam     m_team2;
};

struct ArenaQueueEntry {
    Guid player1 = 0;
    Guid player2 = 0; // 0 if solo queue
    int32 rating = 0;
};

class ArenaMgr {
public:
    static ArenaMgr& instance();

    void loadTemplates();
    void queueSolo(ServerPlayer* player);
    void queueParty(ServerPlayer* leader);
    void dequeue(ServerPlayer* player);
    bool isInQueue(Guid guid) const;

    void update(int32 deltaMs);

private:
    ArenaMgr() = default;
    void tryMatchmaking();
    Arena* createArenaInstance();

    mutable std::mutex m_mutex;
    std::vector<ArenaQueueEntry> m_queue;
    std::vector<std::shared_ptr<Arena>> m_activeArenas;
    std::vector<ArenaTemplate> m_templates;
    uint32 m_nextInstanceId = 1;
};
