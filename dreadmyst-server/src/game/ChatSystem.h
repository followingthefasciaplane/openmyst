#pragma once
#include "core/Types.h"
#include <string>

class ServerPlayer;
class GamePacket;

enum class ChatChannel : uint8 {
    Say     = 0,
    Whisper = 1,
    Party   = 2,
    Guild   = 3,
    AllChat = 4,
    System  = 5
};

namespace ChatSystem {
    void handleChatMessage(ServerPlayer* sender, uint8 channel, const std::string& message, const std::string& target = "");

    void sendSystemMessage(ServerPlayer* player, const std::string& message);
    void sendWhisper(ServerPlayer* from, const std::string& targetName, const std::string& message);
    void broadcastSay(ServerPlayer* sender, const std::string& message);
    void broadcastParty(ServerPlayer* sender, const std::string& message);
    void broadcastGuild(ServerPlayer* sender, const std::string& message);
    void broadcastAllChat(ServerPlayer* sender, const std::string& message);

    // GM commands
    bool processCommand(ServerPlayer* player, const std::string& message);
    bool isCommand(const std::string& message);

    // Build chat packet
    GamePacket buildChatPacket(uint8 channel, const std::string& senderName, const std::string& message);
}
