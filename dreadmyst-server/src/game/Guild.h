#pragma once
#include "core/Types.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

class ServerPlayer;

struct GuildMember {
    Guid   guid     = 0;
    uint32 guildId  = 0;
    uint32 rank     = 0;
    uint8  classId  = 0;
    std::string name;
    uint16 level    = 0;
    bool   online   = false;
};

class Guild {
public:
    Guild(uint32 id, const std::string& name, Guid leaderGuid);

    uint32 getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    Guid getLeaderGuid() const { return m_leaderGuid; }
    const std::string& getMotd() const { return m_motd; }
    int64 getCreateDate() const { return m_createDate; }

    void setMotd(const std::string& motd);
    void setLeader(Guid guid);
    void setCreateDate(int64 date) { m_createDate = date; }

    bool addMember(const GuildMember& member);
    bool removeMember(Guid guid);
    bool hasMember(Guid guid) const;
    GuildMember* getMember(Guid guid);
    const std::vector<GuildMember>& getMembers() const { return m_members; }
    uint32 getMemberCount() const { return static_cast<uint32>(m_members.size()); }

    void setMemberRank(Guid guid, uint32 rank);
    void setMemberOnline(Guid guid, bool online);

    // Packet building
    void sendRoster(ServerPlayer* player);
    void broadcastAddMember(const GuildMember& member);
    void broadcastRemoveMember(Guid guid);
    void broadcastOnlineStatus(Guid guid, bool online);

    // DB
    void saveToDB();
    void deleteFromDB();

private:
    uint32      m_id;
    std::string m_name;
    Guid        m_leaderGuid;
    std::string m_motd;
    int64       m_createDate = 0;
    std::vector<GuildMember> m_members;
};

class GuildMgr {
public:
    static GuildMgr& instance();

    void loadFromDB();

    Guild* createGuild(const std::string& name, ServerPlayer* leader);
    void disbandGuild(uint32 guildId);
    Guild* findGuild(uint32 guildId);
    Guild* findGuildByName(const std::string& name);

    void invitePlayer(ServerPlayer* inviter, const std::string& targetName);
    void acceptInvite(ServerPlayer* player, uint32 guildId);
    void declineInvite(ServerPlayer* player, uint32 guildId);

    void removeMember(uint32 guildId, Guid memberGuid);
    void setMotd(uint32 guildId, const std::string& motd);
    void setMemberRank(uint32 guildId, Guid guid, uint32 rank);

    uint32 getGuildCount() const;
    uint32 getNextGuildId();

private:
    GuildMgr() = default;
    mutable std::mutex m_mutex;
    std::unordered_map<uint32, std::shared_ptr<Guild>> m_guilds;
    uint32 m_nextGuildId = 1;
};
