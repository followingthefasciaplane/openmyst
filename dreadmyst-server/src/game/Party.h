#pragma once
#include "core/Types.h"
#include <vector>
#include <memory>

class ServerPlayer;

class ServerParty {
public:
    ServerParty(Guid leaderGuid);

    Guid getLeaderGuid() const { return m_leaderGuid; }
    void setLeader(Guid guid);

    bool addMember(Guid guid);
    bool removeMember(Guid guid);
    bool hasMember(Guid guid) const;
    uint32 getMemberCount() const { return static_cast<uint32>(m_members.size()); }
    const std::vector<Guid>& getMembers() const { return m_members; }

    // XP distribution (from binary strings)
    float getXpDivider() const;
    std::string getXpMessage() const;

    // Sync
    void sendPartyList(ServerPlayer* player);
    void broadcastPartyList();

    bool isFull() const { return m_members.size() >= MAX_PARTY_SIZE; }

    static const uint32 MAX_PARTY_SIZE = 4;

private:
    Guid m_leaderGuid = 0;
    std::vector<Guid> m_members;
};

class PartyMgr {
public:
    static PartyMgr& instance();

    ServerParty* createParty(ServerPlayer* leader);
    void disbandParty(ServerParty* party);
    ServerParty* findPartyByMember(Guid guid);

    void invitePlayer(ServerPlayer* inviter, const std::string& targetName);
    void acceptInvite(ServerPlayer* player);
    void declineInvite(ServerPlayer* player);

private:
    PartyMgr() = default;
    std::vector<std::shared_ptr<ServerParty>> m_parties;
};
