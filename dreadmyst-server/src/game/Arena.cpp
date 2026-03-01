// Arena.cpp - Arena PvP system, matchmaking, and queue management
// Binary strings verified:
//   "[ARENA]: Match begins in 60 seconds." / 45 / 30 / 15
//   "[ARENA]: The match has begun!"
//   "[ARENA]: Match ended in a draw."
//   "[ARENA]: Victory! Rating: " / "[ARENA]: Defeat. Rating: "
//   "You are not in the arena queue."
//   "You are already in the arena queue."
//   "A party member is already in the arena queue."
//   "You have joined the 2v2 arena queue (solo)."
//   "[ARENA]: Your party has joined the 2v2 arena queue."
//   "[ARENA]: Your partner left. You have been removed from the arena queue."
//   "The bracket is 2v2. Your party has too many members."

#include "game/Arena.h"
#include "game/ServerPlayer.h"
#include "game/PlayerMgr.h"
#include "game/Party.h"
#include "game/ChatSystem.h"
#include "game/GameCache.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"
#include "core/Log.h"

#include <algorithm>
#include <cmath>

extern GameCache& getGameCache();

// ============================================================================
//  Arena
// ============================================================================

Arena::Arena(const ArenaTemplate& tmpl, uint32 instanceId)
    : m_template(tmpl)
    , m_instanceId(instanceId) {
}

void Arena::addTeam1Player(Guid guid) {
    m_team1.playerGuids.push_back(guid);
}

void Arena::addTeam2Player(Guid guid) {
    m_team2.playerGuids.push_back(guid);
}

void Arena::start() {
    m_state = ArenaState::Countdown60;
    m_countdownTimer = 60000; // 60 seconds in ms

    broadcastMessage("[ARENA]: Match begins in 60 seconds.");
    teleportPlayersToStartPositions();
}

void Arena::update(int32 deltaMs) {
    if (m_state == ArenaState::Ended || m_state == ArenaState::Waiting) {
        return;
    }

    if (m_state == ArenaState::InProgress) {
        // Check for team elimination
        if (isTeamEliminated(1)) {
            endMatch(2); // Team 2 wins
        } else if (isTeamEliminated(2)) {
            endMatch(1); // Team 1 wins
        }
        return;
    }

    // Countdown phase
    int32 prevTimer = m_countdownTimer;
    m_countdownTimer -= deltaMs;

    // Transition through countdown states and broadcast messages at thresholds
    if (prevTimer > 45000 && m_countdownTimer <= 45000 && m_state == ArenaState::Countdown60) {
        m_state = ArenaState::Countdown45;
        broadcastMessage("[ARENA]: Match begins in 45 seconds.");
    }
    if (prevTimer > 30000 && m_countdownTimer <= 30000 && m_state == ArenaState::Countdown45) {
        m_state = ArenaState::Countdown30;
        broadcastMessage("[ARENA]: Match begins in 30 seconds.");
    }
    if (prevTimer > 15000 && m_countdownTimer <= 15000 && m_state == ArenaState::Countdown30) {
        m_state = ArenaState::Countdown15;
        broadcastMessage("[ARENA]: Match begins in 15 seconds.");
    }
    if (m_countdownTimer <= 0) {
        m_state = ArenaState::InProgress;
        m_countdownTimer = 0;
        broadcastMessage("[ARENA]: The match has begun!");
    }
}

void Arena::endMatch(int winningTeam) {
    m_state = ArenaState::Ended;

    if (winningTeam == 0) {
        broadcastMessage("[ARENA]: Match ended in a draw.");
    } else {
        awardRating(winningTeam);
    }
}

void Arena::onPlayerDeath(Guid guid) {
    if (m_state != ArenaState::InProgress) return;

    // Check if either team is fully eliminated
    if (isTeamEliminated(1)) {
        endMatch(2);
    } else if (isTeamEliminated(2)) {
        endMatch(1);
    }
}

void Arena::onPlayerDisconnect(Guid guid) {
    // Treat disconnect as death/elimination for that player
    // If a team is now eliminated, end the match

    // Notify the partner
    bool wasTeam1 = std::find(m_team1.playerGuids.begin(), m_team1.playerGuids.end(), guid)
                    != m_team1.playerGuids.end();
    bool wasTeam2 = std::find(m_team2.playerGuids.begin(), m_team2.playerGuids.end(), guid)
                    != m_team2.playerGuids.end();

    if (wasTeam1) {
        // Notify team 1 partner
        for (Guid teamGuid : m_team1.playerGuids) {
            if (teamGuid == guid) continue;
            ServerPlayer* partner = PlayerMgr::instance().findPlayer(teamGuid);
            if (partner) {
                ChatSystem::sendSystemMessage(partner,
                    "[ARENA]: Your partner left. You have been removed from the arena queue.");
            }
        }
        // Team 1 loses
        if (m_state == ArenaState::InProgress) {
            endMatch(2);
        }
    } else if (wasTeam2) {
        for (Guid teamGuid : m_team2.playerGuids) {
            if (teamGuid == guid) continue;
            ServerPlayer* partner = PlayerMgr::instance().findPlayer(teamGuid);
            if (partner) {
                ChatSystem::sendSystemMessage(partner,
                    "[ARENA]: Your partner left. You have been removed from the arena queue.");
            }
        }
        if (m_state == ArenaState::InProgress) {
            endMatch(1);
        }
    }
}

void Arena::broadcastMessage(const std::string& msg) {
    // Send to all players in both teams
    for (Guid guid : m_team1.playerGuids) {
        ServerPlayer* player = PlayerMgr::instance().findPlayer(guid);
        if (player) {
            ChatSystem::sendSystemMessage(player, msg);
        }
    }
    for (Guid guid : m_team2.playerGuids) {
        ServerPlayer* player = PlayerMgr::instance().findPlayer(guid);
        if (player) {
            ChatSystem::sendSystemMessage(player, msg);
        }
    }
}

void Arena::teleportPlayersToStartPositions() {
    // Team 1 positions: player1 and player2 start positions
    if (m_team1.playerGuids.size() > 0) {
        ServerPlayer* p = PlayerMgr::instance().findPlayer(m_team1.playerGuids[0]);
        if (p) {
            p->setMapId(m_template.map);
            p->setPosition(m_template.player1XStart, m_template.player1YStart);
        }
    }
    if (m_team1.playerGuids.size() > 1) {
        ServerPlayer* p = PlayerMgr::instance().findPlayer(m_team1.playerGuids[1]);
        if (p) {
            p->setMapId(m_template.map);
            p->setPosition(m_template.player2XStart, m_template.player2YStart);
        }
    }

    // Team 2 positions: player3 and player4 start positions
    if (m_team2.playerGuids.size() > 0) {
        ServerPlayer* p = PlayerMgr::instance().findPlayer(m_team2.playerGuids[0]);
        if (p) {
            p->setMapId(m_template.map);
            p->setPosition(m_template.player3XStart, m_template.player3YStart);
        }
    }
    if (m_team2.playerGuids.size() > 1) {
        ServerPlayer* p = PlayerMgr::instance().findPlayer(m_team2.playerGuids[1]);
        if (p) {
            p->setMapId(m_template.map);
            p->setPosition(m_template.player4XStart, m_template.player4YStart);
        }
    }
}

void Arena::awardRating(int winningTeam) {
    // Base rating change: 15 points
    int32 ratingChange = 15;

    ArenaTeam& winners = (winningTeam == 1) ? m_team1 : m_team2;
    ArenaTeam& losers  = (winningTeam == 1) ? m_team2 : m_team1;

    // Award rating to winners
    for (Guid guid : winners.playerGuids) {
        ServerPlayer* player = PlayerMgr::instance().findPlayer(guid);
        if (player) {
            int32 newRating = static_cast<int32>(player->getCombatRating()) + ratingChange;
            if (newRating < 0) newRating = 0;
            player->setCombatRating(static_cast<uint32>(newRating));
            ChatSystem::sendSystemMessage(player,
                "[ARENA]: Victory! Rating: " + std::to_string(newRating) +
                " (+" + std::to_string(ratingChange) + ")");
        }
    }

    // Deduct rating from losers
    for (Guid guid : losers.playerGuids) {
        ServerPlayer* player = PlayerMgr::instance().findPlayer(guid);
        if (player) {
            int32 newRating = static_cast<int32>(player->getCombatRating()) - ratingChange;
            if (newRating < 0) newRating = 0;
            player->setCombatRating(static_cast<uint32>(newRating));
            ChatSystem::sendSystemMessage(player,
                "[ARENA]: Defeat. Rating: " + std::to_string(newRating) +
                " (-" + std::to_string(ratingChange) + ")");
        }
    }
}

bool Arena::isTeamEliminated(int team) const {
    const ArenaTeam& t = (team == 1) ? m_team1 : m_team2;

    for (Guid guid : t.playerGuids) {
        ServerPlayer* player = PlayerMgr::instance().findPlayer(guid);
        if (player && player->isAlive()) {
            return false; // At least one player alive
        }
    }
    return true; // All dead or disconnected
}

// ============================================================================
//  ArenaMgr
// ============================================================================

ArenaMgr& ArenaMgr::instance() {
    static ArenaMgr s_instance;
    return s_instance;
}

void ArenaMgr::loadTemplates() {
    const GameCache& cache = getGameCache();
    const ArenaTemplate* tmpl = cache.getArenaTemplate();
    if (tmpl) {
        m_templates.push_back(*tmpl);
    }
    LOG_INFO("ArenaMgr: Loaded %zu arena template(s)", m_templates.size());
}

void ArenaMgr::queueSolo(ServerPlayer* player) {
    if (!player) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    Guid guid = player->getGuid();

    // Check if already in queue
    if (isInQueue(guid)) {
        ChatSystem::sendSystemMessage(player, "You are already in the arena queue.");
        return;
    }

    ArenaQueueEntry entry;
    entry.player1 = guid;
    entry.player2 = 0; // solo
    entry.rating = static_cast<int32>(player->getCombatRating());

    m_queue.push_back(entry);
    ChatSystem::sendSystemMessage(player, "You have joined the 2v2 arena queue (solo).");

    LOG_INFO("ArenaMgr: Player '%s' (guid %u, rating %d) queued solo",
             player->getName().c_str(), guid, entry.rating);
}

void ArenaMgr::queueParty(ServerPlayer* leader) {
    if (!leader) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    ServerParty* party = PartyMgr::instance().findPartyByMember(leader->getGuid());
    if (!party) {
        ChatSystem::sendSystemMessage(leader, "You are not in a party.");
        return;
    }

    // 2v2 bracket: party must have exactly 2 members
    if (party->getMemberCount() > 2) {
        ChatSystem::sendSystemMessage(leader, "The bracket is 2v2. Your party has too many members.");
        return;
    }

    // Check that no party member is already queued
    for (Guid memberGuid : party->getMembers()) {
        if (isInQueue(memberGuid)) {
            ServerPlayer* member = PlayerMgr::instance().findPlayer(memberGuid);
            std::string memberName = member ? member->getName() : "Unknown";
            ChatSystem::sendSystemMessage(leader, "A party member is already in the arena queue.");
            return;
        }
    }

    // Build queue entry
    ArenaQueueEntry entry;
    const auto& members = party->getMembers();
    entry.player1 = members[0];
    entry.player2 = (members.size() > 1) ? members[1] : 0;

    // Average rating of the party
    int32 totalRating = 0;
    int32 ratingCount = 0;
    for (Guid memberGuid : members) {
        ServerPlayer* member = PlayerMgr::instance().findPlayer(memberGuid);
        if (member) {
            totalRating += static_cast<int32>(member->getCombatRating());
            ratingCount++;
        }
    }
    entry.rating = (ratingCount > 0) ? (totalRating / ratingCount) : 0;

    m_queue.push_back(entry);

    // Notify all party members
    for (Guid memberGuid : members) {
        ServerPlayer* member = PlayerMgr::instance().findPlayer(memberGuid);
        if (member) {
            ChatSystem::sendSystemMessage(member, "[ARENA]: Your party has joined the 2v2 arena queue.");
        }
    }

    LOG_INFO("ArenaMgr: Party (leader guid %u, rating %d) queued for 2v2",
             leader->getGuid(), entry.rating);
}

void ArenaMgr::dequeue(ServerPlayer* player) {
    if (!player) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    Guid guid = player->getGuid();
    bool found = false;

    auto it = m_queue.begin();
    while (it != m_queue.end()) {
        if (it->player1 == guid || it->player2 == guid) {
            // Notify partner if exists
            Guid partnerGuid = (it->player1 == guid) ? it->player2 : it->player1;
            if (partnerGuid != 0) {
                ServerPlayer* partner = PlayerMgr::instance().findPlayer(partnerGuid);
                if (partner) {
                    ChatSystem::sendSystemMessage(partner,
                        "[ARENA]: Your partner left. You have been removed from the arena queue.");
                }
            }

            it = m_queue.erase(it);
            found = true;
        } else {
            ++it;
        }
    }

    if (!found) {
        ChatSystem::sendSystemMessage(player, "You are not in the arena queue.");
    }
}

bool ArenaMgr::isInQueue(Guid guid) const {
    for (const auto& entry : m_queue) {
        if (entry.player1 == guid || entry.player2 == guid) {
            return true;
        }
    }
    return false;
}

void ArenaMgr::update(int32 deltaMs) {
    // Update active arenas
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto it = m_activeArenas.begin(); it != m_activeArenas.end(); ) {
            (*it)->update(deltaMs);
            if ((*it)->getState() == ArenaState::Ended) {
                it = m_activeArenas.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Try to create new matches from the queue
    tryMatchmaking();
}

void ArenaMgr::tryMatchmaking() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_queue.size() < 2) return;
    if (m_templates.empty()) return;

    // Sort queue by rating for better matching
    std::sort(m_queue.begin(), m_queue.end(),
        [](const ArenaQueueEntry& a, const ArenaQueueEntry& b) {
            return a.rating < b.rating;
        });

    // Try to match adjacent entries (closest rating)
    for (size_t i = 0; i + 1 < m_queue.size(); ++i) {
        ArenaQueueEntry& entry1 = m_queue[i];
        ArenaQueueEntry& entry2 = m_queue[i + 1];

        // Rating tolerance: match if within 200 rating of each other
        int32 ratingDiff = std::abs(entry1.rating - entry2.rating);
        if (ratingDiff > 200) continue;

        // Create arena instance
        Arena* arena = createArenaInstance();
        if (!arena) continue;

        // Assign teams
        arena->addTeam1Player(entry1.player1);
        if (entry1.player2 != 0) {
            arena->addTeam1Player(entry1.player2);
        }

        arena->addTeam2Player(entry2.player1);
        if (entry2.player2 != 0) {
            arena->addTeam2Player(entry2.player2);
        }

        // Start the match
        arena->start();

        // Remove matched entries from queue (remove higher index first)
        m_queue.erase(m_queue.begin() + static_cast<ptrdiff_t>(i + 1));
        m_queue.erase(m_queue.begin() + static_cast<ptrdiff_t>(i));

        LOG_INFO("ArenaMgr: Created arena instance %u (rating ~%d vs ~%d)",
                 arena->getInstanceId(), entry1.rating, entry2.rating);

        // Only match one pair per tick to avoid invalidating iterators
        break;
    }
}

Arena* ArenaMgr::createArenaInstance() {
    if (m_templates.empty()) return nullptr;

    // Pick first available template (or random if multiple exist)
    const ArenaTemplate& tmpl = m_templates[0];
    uint32 instanceId = m_nextInstanceId++;

    auto arena = std::make_shared<Arena>(tmpl, instanceId);
    m_activeArenas.push_back(arena);

    return arena.get();
}
