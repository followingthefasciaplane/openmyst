#pragma once

#include <string>
#include <cstdint>

// PacketSender - builds and sends outbound packets to the game server.
//
// Binary references:
//   GP_Client_Authenticate::build: FUN_00419160 at 0x00419160
//   Login flow (sends auth): FUN_0049d5e0 at 0x0049d5e0
//   Generic sendPacket: FUN_00424e60 at 0x00424e60
//
// Wire format matches reimplemented server's PacketBuffer:
//   - Strings: uint16 length + data (NOT null-terminated)
//   - Integers: little-endian
//   - sf::Packet handles framing (4-byte BE size header)

namespace PacketSender
{
    // Phase 1: Authentication
    // Wire opcode: 0x02 (server dispatch case 0x01)
    // Payload: [string authToken (len-prefixed)][uint32 clientVersion]
    void sendAuthenticate(const std::string& authToken, uint32_t version);

    // Phase 2: Request character list
    // Wire opcode: 0x06 (server dispatch case 0x05)
    // Payload: empty (server reads account from session)
    void sendRequestCharList();

    // Phase 2: Create character
    // Wire opcode: 0x04 (server dispatch case 0x03)
    // Payload: [string name][uint8 classId][uint8 gender][uint8 portrait]
    void sendCharCreate(const std::string& name, uint8_t classId, uint8_t gender, uint8_t portrait);

    // Phase 2: Delete character
    // Wire opcode: 0x07 (server dispatch case 0x06)
    // Payload: [uint32 charGuid][uint32 unused]
    void sendDeleteChar(uint32_t charGuid);

    // Phase 2: Enter world with selected character
    // Wire opcode: 0x05 (server dispatch case 0x04)
    // Payload: [uint32 charGuid]
    void sendEnterWorld(uint32_t charGuid);

    // Phase 4: Request move
    // Wire opcode: 0x09 (server dispatch case 0x08)
    // Payload: [float posX][float posY][float orientation]
    void sendRequestMove(float posX, float posY, float orientation);

    // Phase 4: Set target
    // Wire opcode: 0x0B (server dispatch case 0x0A)
    // Payload: [uint32 targetGuid]
    void sendSetTarget(uint32_t targetGuid);

    // Phase 4: Chat message
    // Wire opcode: 0x08 (server dispatch case 0x07)
    // Payload: [uint8 channel][string message]
    void sendChatMessage(uint8_t channel, const std::string& message);

    // Login with username+password (bypasses token auth)
    // Wire opcode: 0x4B (server dispatch case 0x4A)
    // Payload: [string username][string password][uint32 version]
    void sendLogin(const std::string& username, const std::string& password, uint32_t version);
}
