// Party.cpp - Party system and party manager
// Binary strings:
//   "ServerParty::setLeader - Tried to set leader but he's not in our party (Name '%s', Account '%d')"
//   "Full party, kill XP divided by (%d)."
//   "Medium party, kill XP divided by (%d)."
//   "Light party, kill XP received is slightly reduced."
//   "ServerPlayer::registerParty - Should not be overwriting existing party pointer... error??"
//   "The bracket is 2v2. Your party has too many members."

#include "game/Party.h"
#include "game/ServerPlayer.h"
#include "game/PlayerMgr.h"
#include "game/ChatSystem.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"
#include "core/Log.h"

#include <algorithm>

// ============================================================================
//  ServerParty
// ============================================================================

ServerParty::ServerParty(Guid leaderGuid)
    : m_leaderGuid(leaderGuid) {
    m_members.push_back(leaderGuid);
}

void ServerParty::setLeader(Guid guid) {
    // Verify the guid is actually in our party
    if (!hasMember(guid)) {
        ServerPlayer* player = PlayerMgr::instance().findPlayer(guid);
        const char* name = player ? player->getName().c_str() : "Unknown";
        uint32 accountId = player ? player->getAccountId() : 0;
        LOG_ERROR("ServerParty::setLeader - Tried to set leader but he's not in our party (Name '%s', Account '%d')",
                  name, accountId);
        return;
    }

    m_leaderGuid = guid;
    broadcastPartyList();
}

bool ServerParty::addMember(Guid guid) {
    if (isFull()) return false;
    if (hasMember(guid)) return false;

    m_members.push_back(guid);
    broadcastPartyList();
    return true;
}

bool ServerParty::removeMember(Guid guid) {
    auto it = std::find(m_members.begin(), m_members.end(), guid);
    if (it == m_members.end()) return false;

    m_members.erase(it);

    // If the leader left, promote the next member
    if (guid == m_leaderGuid && !m_members.empty()) {
        m_leaderGuid = m_members[0];
    }

    broadcastPartyList();
    return true;
}

bool ServerParty::hasMember(Guid guid) const {
    return std::find(m_members.begin(), m_members.end(), guid) != m_members.end();
}

float ServerParty::getXpDivider() const {
    uint32 count = getMemberCount();
    switch (count) {
        case 4: return static_cast<float>(count);  // Full party
        case 3: return static_cast<float>(count);  // Medium party
        case 2: return 1.5f;                        // Light party (slight reduction)
        default: return 1.0f;                       // Solo, no division
    }
}

std::string ServerParty::getXpMessage() const {
    uint32 count = getMemberCount();
    switch (count) {
        case 4: {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "Full party, kill XP divided by (%d).", count);
            return std::string(buf);
        }
        case 3: {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "Medium party, kill XP divided by (%d).", count);
            return std::string(buf);
        }
        case 2:
            return "Light party, kill XP received is slightly reduced.";
        default:
            return "";
    }
}

void ServerParty::sendPartyList(ServerPlayer* player) {
    if (!player) return;

    GamePacket pkt(SMSG_PARTY_LIST);
    pkt.writeUint32(static_cast<uint32>(m_members.size()));
    pkt.writeUint32(m_leaderGuid);

    for (Guid memberGuid : m_members) {
        pkt.writeUint32(memberGuid);

        ServerPlayer* member = PlayerMgr::instance().findPlayer(memberGuid);
        if (member) {
            pkt.writeString(member->getName());
            pkt.writeUint16(member->getLevel());
            pkt.writeUint8(member->getClassId());
            pkt.writeInt32(member->getHealth());
            pkt.writeInt32(member->getMaxHealth());
            pkt.writeInt32(member->getMana());
            pkt.writeInt32(member->getMaxMana());
        } else {
            pkt.writeString("Offline");
            pkt.writeUint16(0);
            pkt.writeUint8(0);
            pkt.writeInt32(0);
            pkt.writeInt32(0);
            pkt.writeInt32(0);
            pkt.writeInt32(0);
        }
    }

    player->sendPacket(pkt);
}

void ServerParty::broadcastPartyList() {
    for (Guid memberGuid : m_members) {
        ServerPlayer* member = PlayerMgr::instance().findPlayer(memberGuid);
        if (member) {
            sendPartyList(member);
        }
    }
}

// ============================================================================
//  PartyMgr
// ============================================================================

PartyMgr& PartyMgr::instance() {
    static PartyMgr s_instance;
    return s_instance;
}

ServerParty* PartyMgr::createParty(ServerPlayer* leader) {
    if (!leader) return nullptr;

    // Check if leader is already in a party
    if (findPartyByMember(leader->getGuid())) {
        LOG_WARN("ServerPlayer::registerParty - Should not be overwriting existing party pointer... error??");
        return nullptr;
    }

    auto party = std::make_shared<ServerParty>(leader->getGuid());
    m_parties.push_back(party);

    party->broadcastPartyList();

    return party.get();
}

void PartyMgr::disbandParty(ServerParty* party) {
    if (!party) return;

    // Notify all members that the party has been disbanded
    for (Guid memberGuid : party->getMembers()) {
        ServerPlayer* member = PlayerMgr::instance().findPlayer(memberGuid);
        if (member) {
            // Send empty party list to indicate no party
            GamePacket pkt(SMSG_PARTY_LIST);
            pkt.writeUint32(0); // 0 members = no party
            pkt.writeUint32(0);
            member->sendPacket(pkt);
        }
    }

    // Remove from tracked parties
    auto it = std::find_if(m_parties.begin(), m_parties.end(),
        [party](const std::shared_ptr<ServerParty>& p) { return p.get() == party; });
    if (it != m_parties.end()) {
        m_parties.erase(it);
    }
}

ServerParty* PartyMgr::findPartyByMember(Guid guid) {
    for (auto& party : m_parties) {
        if (party->hasMember(guid)) {
            return party.get();
        }
    }
    return nullptr;
}

void PartyMgr::invitePlayer(ServerPlayer* inviter, const std::string& targetName) {
    if (!inviter) return;

    ServerPlayer* target = PlayerMgr::instance().findPlayerByName(targetName);
    if (!target) {
        ChatSystem::sendSystemMessage(inviter, "Player '" + targetName + "' is not online.");
        return;
    }

    if (target->getGuid() == inviter->getGuid()) {
        ChatSystem::sendSystemMessage(inviter, "You cannot invite yourself.");
        return;
    }

    // Check if target is already in a party
    if (findPartyByMember(target->getGuid())) {
        ChatSystem::sendSystemMessage(inviter, targetName + " is already in a party.");
        return;
    }

    // Check if inviter's party is full
    ServerParty* inviterParty = findPartyByMember(inviter->getGuid());
    if (inviterParty && inviterParty->isFull()) {
        ChatSystem::sendSystemMessage(inviter, "Your party is full.");
        return;
    }

    // Send party invitation to target
    GamePacket pkt(SMSG_OFFER_PARTY);
    pkt.writeUint32(inviter->getGuid());
    pkt.writeString(inviter->getName());
    target->sendPacket(pkt);

    ChatSystem::sendSystemMessage(inviter, "Party invite sent to " + targetName + ".");
}

void PartyMgr::acceptInvite(ServerPlayer* player) {
    if (!player) return;

    // In a full implementation, we would track pending invites per player
    // and look up who invited them. For now, this creates or joins the
    // inviter's party. The invite tracking would store the inviter's guid.
    // Simplified: the session layer should pass the inviter guid.

    // If player is already in a party, warn
    if (findPartyByMember(player->getGuid())) {
        LOG_WARN("ServerPlayer::registerParty - Should not be overwriting existing party pointer... error??");
        return;
    }

    // The actual party join is handled by the invite accept handler
    // which would call party->addMember after validating the invite
}

void PartyMgr::declineInvite(ServerPlayer* player) {
    if (!player) return;
    // In a full implementation, notify the inviter that the invite was declined
    // and clear the pending invite state
}
