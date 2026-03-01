#include "game/Guild.h"
#include "game/ServerPlayer.h"
#include "game/PlayerMgr.h"
#include "network/Opcodes.h"
#include "network/GamePacket.h"
#include "database/DatabaseMgr.h"
#include "core/Log.h"

#include <algorithm>
#include <sstream>

// ============================================================================
// Guild
// ============================================================================

Guild::Guild(uint32 id, const std::string& name, Guid leaderGuid)
    : m_id(id), m_name(name), m_leaderGuid(leaderGuid) {
}

void Guild::setMotd(const std::string& motd) {
    m_motd = motd;
}

void Guild::setLeader(Guid guid) {
    m_leaderGuid = guid;
}

bool Guild::addMember(const GuildMember& member) {
    // Check for duplicate
    if (hasMember(member.guid)) {
        LOG_WARN("Guild::addMember guild=%u member=%u already exists", m_id, member.guid);
        return false;
    }

    m_members.push_back(member);

    // Persist to DB: REPLACE INTO guild_member (guid, id, rank) VALUES(...)
    auto& db = DatabaseMgr::instance().getPlayerConnection();
    std::ostringstream sql;
    sql << "REPLACE INTO guild_member (guid, id, rank) VALUES("
        << member.guid << ", " << m_id << ", " << member.rank << ")";
    db.execute(sql.str());

    LOG_INFO("Guild::addMember guild=%u (%s) member=%u (%s)",
             m_id, m_name.c_str(), member.guid, member.name.c_str());
    return true;
}

bool Guild::removeMember(Guid guid) {
    auto it = std::find_if(m_members.begin(), m_members.end(),
        [guid](const GuildMember& m) { return m.guid == guid; });

    if (it == m_members.end()) {
        return false;
    }

    m_members.erase(it);

    // Persist to DB: DELETE FROM guild_member WHERE guid = X
    auto& db = DatabaseMgr::instance().getPlayerConnection();
    std::ostringstream sql;
    sql << "DELETE FROM guild_member WHERE guid = " << guid;
    db.execute(sql.str());

    LOG_INFO("Guild::removeMember guild=%u member=%u", m_id, guid);
    return true;
}

bool Guild::hasMember(Guid guid) const {
    for (const auto& m : m_members) {
        if (m.guid == guid) return true;
    }
    return false;
}

GuildMember* Guild::getMember(Guid guid) {
    for (auto& m : m_members) {
        if (m.guid == guid) return &m;
    }
    return nullptr;
}

void Guild::setMemberRank(Guid guid, uint32 rank) {
    GuildMember* member = getMember(guid);
    if (!member) return;

    member->rank = rank;

    // Persist to DB: REPLACE INTO guild_member (guid, id, rank) VALUES(...)
    auto& db = DatabaseMgr::instance().getPlayerConnection();
    std::ostringstream sql;
    sql << "REPLACE INTO guild_member (guid, id, rank) VALUES("
        << guid << ", " << m_id << ", " << rank << ")";
    db.execute(sql.str());

    // Notify online guild members
    GamePacket packet(SMSG_GUILD_NOTIFY_ROLE_CHANGE);
    packet.writeUint32(guid);
    packet.writeUint32(rank);

    for (const auto& m : m_members) {
        if (m.online) {
            ServerPlayer* p = PlayerMgr::instance().findPlayer(m.guid);
            if (p) p->sendPacket(packet);
        }
    }
}

void Guild::setMemberOnline(Guid guid, bool online) {
    GuildMember* member = getMember(guid);
    if (!member) return;

    member->online = online;
    broadcastOnlineStatus(guid, online);
}

// -- Packet building --

void Guild::sendRoster(ServerPlayer* player) {
    if (!player) return;

    GamePacket packet(SMSG_GUILD_ROSTER);
    packet.writeUint32(m_id);
    packet.writeString(m_name);
    packet.writeUint32(m_leaderGuid);
    packet.writeString(m_motd);
    packet.writeUint32(getMemberCount());

    for (const auto& m : m_members) {
        packet.writeUint32(m.guid);
        packet.writeUint32(m.rank);
        packet.writeUint8(m.classId);
        packet.writeString(m.name);
        packet.writeUint16(m.level);
        packet.writeUint8(m.online ? 1 : 0);
    }

    player->sendPacket(packet);
}

void Guild::broadcastAddMember(const GuildMember& member) {
    GamePacket packet(SMSG_GUILD_ADD_MEMBER);
    packet.writeUint32(member.guid);
    packet.writeUint32(member.rank);
    packet.writeUint8(member.classId);
    packet.writeString(member.name);
    packet.writeUint16(member.level);
    packet.writeUint8(member.online ? 1 : 0);

    for (const auto& m : m_members) {
        if (m.online) {
            ServerPlayer* p = PlayerMgr::instance().findPlayer(m.guid);
            if (p) p->sendPacket(packet);
        }
    }
}

void Guild::broadcastRemoveMember(Guid guid) {
    GamePacket packet(SMSG_GUILD_REMOVE_MEMBER);
    packet.writeUint32(guid);

    for (const auto& m : m_members) {
        if (m.online) {
            ServerPlayer* p = PlayerMgr::instance().findPlayer(m.guid);
            if (p) p->sendPacket(packet);
        }
    }
}

void Guild::broadcastOnlineStatus(Guid guid, bool online) {
    GamePacket packet(SMSG_GUILD_ONLINE_STATUS);
    packet.writeUint32(guid);
    packet.writeUint8(online ? 1 : 0);

    for (const auto& m : m_members) {
        if (m.online && m.guid != guid) {
            ServerPlayer* p = PlayerMgr::instance().findPlayer(m.guid);
            if (p) p->sendPacket(packet);
        }
    }
}

// -- DB persistence --

void Guild::saveToDB() {
    auto& db = DatabaseMgr::instance().getPlayerConnection();

    // REPLACE INTO guild (id, name, leader_guid, motd, create_date) VALUES(...)
    std::ostringstream sql;
    sql << "REPLACE INTO guild (id, name, leader_guid, motd, create_date) VALUES("
        << m_id << ", '"
        << db.escapeString(m_name) << "', "
        << m_leaderGuid << ", '"
        << db.escapeString(m_motd) << "', "
        << m_createDate << ")";
    db.execute(sql.str());

    // Save all members
    for (const auto& m : m_members) {
        std::ostringstream memberSql;
        memberSql << "REPLACE INTO guild_member (guid, id, rank) VALUES("
                  << m.guid << ", " << m_id << ", " << m.rank << ")";
        db.execute(memberSql.str());
    }
}

void Guild::deleteFromDB() {
    auto& db = DatabaseMgr::instance().getPlayerConnection();

    // DELETE FROM guild_member WHERE id = X
    std::ostringstream memberSql;
    memberSql << "DELETE FROM guild_member WHERE id = " << m_id;
    db.execute(memberSql.str());

    // DELETE FROM guild WHERE id = X
    std::ostringstream guildSql;
    guildSql << "DELETE FROM guild WHERE id = " << m_id;
    db.execute(guildSql.str());
}

// ============================================================================
// GuildMgr
// ============================================================================

GuildMgr& GuildMgr::instance() {
    static GuildMgr inst;
    return inst;
}

void GuildMgr::loadFromDB() {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto& db = DatabaseMgr::instance().getPlayerConnection();

    // Clean orphaned guild_member rows first
    // DELETE FROM guild_member WHERE id NOT IN (SELECT id FROM guild)
    db.execute("DELETE FROM guild_member WHERE id NOT IN (SELECT id FROM guild)");

    // Load guilds: SELECT id, name, leader_guid, motd, create_date FROM guild
    ResultSet guildResults;
    if (!db.query("SELECT id, name, leader_guid, motd, create_date FROM guild", guildResults)) {
        LOG_ERROR("GuildMgr::loadFromDB failed to load guilds");
        return;
    }

    for (const auto& row : guildResults) {
        uint32 id         = static_cast<uint32>(std::stoul(row.at("id")));
        std::string name  = row.at("name");
        Guid leaderGuid   = static_cast<Guid>(std::stoul(row.at("leader_guid")));
        std::string motd  = row.at("motd");
        int64 createDate  = static_cast<int64>(std::stoll(row.at("create_date")));

        auto guild = std::make_shared<Guild>(id, name, leaderGuid);
        guild->setMotd(motd);
        guild->setCreateDate(createDate);
        m_guilds[id] = guild;

        if (id >= m_nextGuildId) {
            m_nextGuildId = id + 1;
        }
    }

    // Load members: SELECT gm.guid, gm.id, gm.rank, p.class, p.name, p.level
    //               FROM guild_member gm INNER JOIN player p ON gm.guid = p.guid
    ResultSet memberResults;
    if (!db.query("SELECT gm.guid, gm.id, gm.rank, p.class, p.name, p.level "
                  "FROM guild_member gm INNER JOIN player p ON gm.guid = p.guid",
                  memberResults)) {
        LOG_ERROR("GuildMgr::loadFromDB failed to load guild members");
        return;
    }

    for (const auto& row : memberResults) {
        Guid memberGuid  = static_cast<Guid>(std::stoul(row.at("guid")));
        uint32 guildId   = static_cast<uint32>(std::stoul(row.at("id")));
        uint32 rank      = static_cast<uint32>(std::stoul(row.at("rank")));
        uint8 classId    = static_cast<uint8>(std::stoul(row.at("class")));
        std::string name = row.at("name");
        uint16 level     = static_cast<uint16>(std::stoul(row.at("level")));

        auto it = m_guilds.find(guildId);
        if (it == m_guilds.end()) {
            LOG_WARN("GuildMgr::loadFromDB orphan member guid=%u guildId=%u", memberGuid, guildId);
            continue;
        }

        GuildMember member;
        member.guid    = memberGuid;
        member.guildId = guildId;
        member.rank    = rank;
        member.classId = classId;
        member.name    = name;
        member.level   = level;
        member.online  = false;

        // Add directly to avoid DB write-back during load
        it->second->getMembers();  // ensure vector exists
        // Use the Guild's internal vector directly during load to avoid
        // re-persisting to DB (addMember writes to DB)
        GuildMember gm = member;
        // We need to bypass addMember's DB write; add to vector directly
        // Since getMembers() is const, we use addMember but accept the
        // redundant REPLACE which is idempotent
        it->second->addMember(member);
    }

    LOG_INFO("GuildMgr::loadFromDB loaded %u guilds", static_cast<uint32>(m_guilds.size()));
}

Guild* GuildMgr::createGuild(const std::string& name, ServerPlayer* leader) {
    if (!leader) return nullptr;

    // Check if player is already in a guild
    if (leader->isInGuild()) {
        LOG_WARN("GuildMgr::createGuild player=%u already in guild", leader->getGuid());
        return nullptr;
    }

    // Check for duplicate name
    if (findGuildByName(name)) {
        LOG_WARN("GuildMgr::createGuild name '%s' already exists", name.c_str());
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    uint32 guildId = m_nextGuildId++;
    auto guild = std::make_shared<Guild>(guildId, name, leader->getGuid());
    guild->setCreateDate(static_cast<int64>(std::time(nullptr)));

    // Add the leader as first member (rank 0 = Guild Master)
    GuildMember leaderMember;
    leaderMember.guid    = leader->getGuid();
    leaderMember.guildId = guildId;
    leaderMember.rank    = 0;
    leaderMember.classId = leader->getClassId();
    leaderMember.name    = leader->getName();
    leaderMember.level   = leader->getLevel();
    leaderMember.online  = true;

    guild->addMember(leaderMember);
    guild->saveToDB();

    m_guilds[guildId] = guild;

    // Update player's guild info
    leader->getGuildInfo().guildId = guildId;
    leader->getGuildInfo().rank    = 0;

    LOG_INFO("GuildMgr::createGuild id=%u name='%s' leader=%u",
             guildId, name.c_str(), leader->getGuid());

    return guild.get();
}

void GuildMgr::disbandGuild(uint32 guildId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_guilds.find(guildId);
    if (it == m_guilds.end()) return;

    Guild* guild = it->second.get();

    // Clear guild info for all online members
    for (const auto& m : guild->getMembers()) {
        ServerPlayer* player = PlayerMgr::instance().findPlayer(m.guid);
        if (player) {
            player->getGuildInfo().guildId = 0;
            player->getGuildInfo().rank    = 0;
        }
    }

    // Broadcast removal to all online members before deleting
    guild->broadcastRemoveMember(0);  // guid 0 signals disband

    // Delete from DB
    guild->deleteFromDB();

    m_guilds.erase(it);

    LOG_INFO("GuildMgr::disbandGuild id=%u", guildId);
}

Guild* GuildMgr::findGuild(uint32 guildId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_guilds.find(guildId);
    if (it != m_guilds.end()) {
        return it->second.get();
    }
    return nullptr;
}

Guild* GuildMgr::findGuildByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [id, guild] : m_guilds) {
        if (guild->getName() == name) {
            return guild.get();
        }
    }
    return nullptr;
}

void GuildMgr::invitePlayer(ServerPlayer* inviter, const std::string& targetName) {
    if (!inviter || !inviter->isInGuild()) return;

    uint32 guildId = inviter->getGuildInfo().guildId;
    Guild* guild = findGuild(guildId);
    if (!guild) return;

    // Only leader (rank 0) or officers (rank 1) can invite
    if (inviter->getGuildInfo().rank > 1) {
        LOG_WARN("GuildMgr::invitePlayer player=%u rank=%u insufficient permissions",
                 inviter->getGuid(), inviter->getGuildInfo().rank);
        return;
    }

    ServerPlayer* target = PlayerMgr::instance().findPlayerByName(targetName);
    if (!target) {
        LOG_DEBUG("GuildMgr::invitePlayer target '%s' not found online", targetName.c_str());
        return;
    }

    if (target->isInGuild()) {
        LOG_DEBUG("GuildMgr::invitePlayer target '%s' already in a guild", targetName.c_str());
        return;
    }

    // Send guild invite packet to target
    GamePacket packet(SMSG_GUILD_INVITE);
    packet.writeUint32(guildId);
    packet.writeString(guild->getName());
    packet.writeUint32(inviter->getGuid());
    packet.writeString(inviter->getName());

    target->sendPacket(packet);

    LOG_INFO("GuildMgr::invitePlayer inviter=%u target='%s' guild=%u",
             inviter->getGuid(), targetName.c_str(), guildId);
}

void GuildMgr::acceptInvite(ServerPlayer* player, uint32 guildId) {
    if (!player) return;

    if (player->isInGuild()) {
        LOG_WARN("GuildMgr::acceptInvite player=%u already in guild", player->getGuid());
        return;
    }

    Guild* guild = findGuild(guildId);
    if (!guild) {
        LOG_WARN("GuildMgr::acceptInvite guild=%u not found", guildId);
        return;
    }

    GuildMember member;
    member.guid    = player->getGuid();
    member.guildId = guildId;
    member.rank    = 3;  // Default rank for new members
    member.classId = player->getClassId();
    member.name    = player->getName();
    member.level   = player->getLevel();
    member.online  = true;

    guild->addMember(member);

    // Update player's guild info
    player->getGuildInfo().guildId = guildId;
    player->getGuildInfo().rank    = member.rank;

    // Broadcast to existing members
    guild->broadcastAddMember(member);

    // Send full roster to new member
    guild->sendRoster(player);

    LOG_INFO("GuildMgr::acceptInvite player=%u joined guild=%u", player->getGuid(), guildId);
}

void GuildMgr::declineInvite(ServerPlayer* player, uint32 guildId) {
    if (!player) return;
    LOG_DEBUG("GuildMgr::declineInvite player=%u guild=%u", player->getGuid(), guildId);
    // No state to clean up; the invite was only a packet
}

void GuildMgr::removeMember(uint32 guildId, Guid memberGuid) {
    Guild* guild = findGuild(guildId);
    if (!guild) return;

    // Cannot remove the leader
    if (guild->getLeaderGuid() == memberGuid) {
        LOG_WARN("GuildMgr::removeMember cannot remove leader guid=%u from guild=%u",
                 memberGuid, guildId);
        return;
    }

    // Broadcast removal before actually removing (so the member still sees it)
    guild->broadcastRemoveMember(memberGuid);
    guild->removeMember(memberGuid);

    // Clear player's guild info if they are online
    ServerPlayer* player = PlayerMgr::instance().findPlayer(memberGuid);
    if (player) {
        player->getGuildInfo().guildId = 0;
        player->getGuildInfo().rank    = 0;
    }

    LOG_INFO("GuildMgr::removeMember guild=%u member=%u", guildId, memberGuid);
}

void GuildMgr::setMotd(uint32 guildId, const std::string& motd) {
    Guild* guild = findGuild(guildId);
    if (!guild) return;

    guild->setMotd(motd);
    guild->saveToDB();

    LOG_INFO("GuildMgr::setMotd guild=%u motd='%s'", guildId, motd.c_str());
}

void GuildMgr::setMemberRank(uint32 guildId, Guid guid, uint32 rank) {
    Guild* guild = findGuild(guildId);
    if (!guild) return;

    guild->setMemberRank(guid, rank);

    // Update online player's cached rank
    ServerPlayer* player = PlayerMgr::instance().findPlayer(guid);
    if (player) {
        player->getGuildInfo().rank = rank;
    }
}

uint32 GuildMgr::getGuildCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<uint32>(m_guilds.size());
}

uint32 GuildMgr::getNextGuildId() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nextGuildId++;
}
