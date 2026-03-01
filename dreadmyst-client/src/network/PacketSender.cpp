#include "PacketSender.h"
#include "PacketHandler.h"
#include "Connection.h"
#include "StlBuffer.h"
#include "core/Log.h"

// PacketSender implementation.
//
// Wire format matches reimplemented server's PacketBuffer:
//   - Strings: uint16 length + data (NOT null-terminated)
//   - Server's handlers read via readString() which is length-prefixed.

// ============================================================================
//  Phase 1: Authentication
// ============================================================================

void PacketSender::sendAuthenticate(const std::string& authToken, uint32_t version)
{
    StlBuffer payload;
    payload.writeLenString(authToken);
    payload.writeUint32(version);

    LOG_INFO("Sending authenticate (token=%s, version=%u)",
             authToken.c_str(), version);

    sConnection.sendPacket(WireOpcode::CMSG_Authenticate, payload);
    sPacketHandler.setAuthState(PacketHandler::AuthState::WaitingForResponse);
}

// ============================================================================
//  Phase 2: Character list / create / delete / enter world
// ============================================================================

void PacketSender::sendRequestCharList()
{
    StlBuffer payload;  // empty payload
    LOG_INFO("Requesting character list");
    sConnection.sendPacket(WireOpcode::CMSG_RequestCharList, payload);
}

void PacketSender::sendCharCreate(const std::string& name, uint8_t classId, uint8_t gender, uint8_t portrait)
{
    StlBuffer payload;
    payload.writeLenString(name);
    payload.writeUint8(classId);
    payload.writeUint8(gender);
    payload.writeUint8(portrait);

    LOG_INFO("Sending char create: name='%s' class=%u gender=%u portrait=%u",
             name.c_str(), classId, gender, portrait);

    sConnection.sendPacket(WireOpcode::CMSG_CharCreate, payload);
}

void PacketSender::sendDeleteChar(uint32_t charGuid)
{
    StlBuffer payload;
    payload.writeUint32(charGuid);
    payload.writeUint32(0);  // unused field (server reads but discards)

    LOG_INFO("Sending delete char: guid=%u", charGuid);

    sConnection.sendPacket(WireOpcode::CMSG_DeleteChar, payload);
}

void PacketSender::sendEnterWorld(uint32_t charGuid)
{
    StlBuffer payload;
    payload.writeUint32(charGuid);

    LOG_INFO("Sending enter world: guid=%u", charGuid);

    sPacketHandler.setSelectedCharGuid(charGuid);
    sPacketHandler.setGameState(GameState::EnteringWorld);
    sConnection.sendPacket(WireOpcode::CMSG_EnterWorld, payload);
}

// ============================================================================
//  Phase 4: Movement + interaction packets
// ============================================================================

void PacketSender::sendRequestMove(float posX, float posY, float orientation)
{
    StlBuffer payload;
    payload.writeFloat(posX);
    payload.writeFloat(posY);
    payload.writeFloat(orientation);

    sConnection.sendPacket(WireOpcode::CMSG_RequestMove, payload);
}

void PacketSender::sendSetTarget(uint32_t targetGuid)
{
    StlBuffer payload;
    payload.writeUint32(targetGuid);

    sConnection.sendPacket(WireOpcode::CMSG_SetTarget, payload);
}

void PacketSender::sendChatMessage(uint8_t channel, const std::string& message)
{
    StlBuffer payload;
    payload.writeUint8(channel);
    payload.writeLenString(message);

    LOG_INFO("Sending chat (channel=%u): %s", channel, message.c_str());
    sConnection.sendPacket(WireOpcode::CMSG_ChatMessage, payload);
}

// ============================================================================
//  Login with username+password (bypasses token auth)
// ============================================================================

void PacketSender::sendLogin(const std::string& username, const std::string& password, uint32_t version)
{
    StlBuffer payload;
    payload.writeLenString(username);
    payload.writeLenString(password);
    payload.writeUint32(version);

    LOG_INFO("Sending login (user=%s, version=%u)", username.c_str(), version);
    sConnection.sendPacket(WireOpcode::CMSG_Login, payload);
    sPacketHandler.setAuthState(PacketHandler::AuthState::WaitingForResponse);
}
