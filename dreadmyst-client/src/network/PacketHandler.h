#pragma once

#include "SfSocket.h"
#include "StlBuffer.h"
#include "CharacterDefines.h"
#include "PlayerDefines.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// PacketHandler - central packet dispatch for incoming server packets.
//
// Binary: processPacket at FUN_00459b80 (0x00459b80)
//   - Reads uint16 opcode, validates range [1, 0x97]
//   - Switches on opcode to call specific handlers
//
// IMPORTANT: The reimplemented server uses DIFFERENT opcode numbering.
//   Server dispatch does switch(opcode - 1), so wire opcodes are:
//     Wire opcode 1 = SMSG_VALIDATE (auth response)
//     Wire opcode 2 = CMSG_AUTHENTICATE (client sends)
//     Wire opcode 3 = SMSG_CHARACTER_LIST
//     etc.
//   These do NOT match Shared/Opcodes.h which has the original binary values.
//
// Server wire opcodes (from server's Opcodes.h and PacketHandler.cpp dispatch):
// Server dispatch does switch(opcode - 1), so wire_opcode = server_case + 1.
namespace WireOpcode
{
    // Server -> Client (SMSG)
    constexpr uint16_t SMSG_Validate             = 0x01;  // Auth response
    constexpr uint16_t SMSG_CharacterList        = 0x03;  // Character list for account
    constexpr uint16_t SMSG_Player               = 0x04;  // Full/partial player snapshot
    constexpr uint16_t SMSG_Unit                 = 0x05;  // NPC/creature data
    constexpr uint16_t SMSG_UnitAuras            = 0x06;  // Active auras on unit
    constexpr uint16_t SMSG_UnitSpline           = 0x07;  // Unit spline movement
    constexpr uint16_t SMSG_Npc                  = 0x08;  // NPC gossip data
    constexpr uint16_t SMSG_Object               = 0x09;  // Generic world object
    constexpr uint16_t SMSG_GameObject           = 0x0A;  // Game object (chest, door)
    constexpr uint16_t SMSG_Inventory            = 0x0B;  // Full inventory (40 slots)
    constexpr uint16_t SMSG_Bank                 = 0x0C;  // Bank contents (40 slots)
    constexpr uint16_t SMSG_SpellbookUpdate      = 0x0E;  // Single spell learned/removed
    constexpr uint16_t SMSG_SpellGo              = 0x0F;  // Spell cast visual + effect
    constexpr uint16_t SMSG_CombatMsg            = 0x10;  // Combat log message
    constexpr uint16_t SMSG_ChatMsg              = 0x11;  // Chat message
    constexpr uint16_t SMSG_QuestList            = 0x13;  // Quest log contents
    constexpr uint16_t SMSG_GossipMenu           = 0x14;  // NPC gossip menu
    constexpr uint16_t SMSG_Spellbook            = 0x50;  // Full spellbook + cooldowns
    constexpr uint16_t SMSG_CharCreate           = 0x51;  // Character creation response
    constexpr uint16_t SMSG_GameStateUpdate1     = 0x4B;  // Position broadcast (movement)
    constexpr uint16_t SMSG_GameState            = 0x52;  // ObjectVariable update

    // Client -> Server (CMSG)
    constexpr uint16_t CMSG_Authenticate         = 0x02;  // server case 0x01
    constexpr uint16_t CMSG_CharCreate           = 0x04;  // server case 0x03
    constexpr uint16_t CMSG_EnterWorld           = 0x05;  // server case 0x04
    constexpr uint16_t CMSG_RequestCharList      = 0x06;  // server case 0x05
    constexpr uint16_t CMSG_DeleteChar           = 0x07;  // server case 0x06
    constexpr uint16_t CMSG_ChatMessage          = 0x08;  // server case 0x07
    constexpr uint16_t CMSG_RequestMove          = 0x09;  // server case 0x08
    constexpr uint16_t CMSG_SetTarget            = 0x0B;  // server case 0x0A
    constexpr uint16_t CMSG_Login                = 0x4B;  // server case 0x4A
}

// Game state machine: tracks client progression through login/char select/world
enum class GameState
{
    Login,            // Connecting + authenticating
    CharSelect,       // Viewing character list, creating/deleting
    EnteringWorld,    // Sent enter world, waiting for SMSG_PLAYER
    InWorld,          // Received player data, in the game world
};

// Number of stat types in SMSG_PLAYER (server's StatType::Count)
constexpr int STAT_TYPE_COUNT = 7;
constexpr int MAX_INVENTORY_SLOTS = 40;
constexpr int MAX_BANK_SLOTS = 40;
constexpr int MAX_QUEST_OBJECTIVES = 4;

// Item instance data (from SMSG_INVENTORY / SMSG_BANK)
struct ItemInstance
{
    uint32_t entry        = 0;
    uint32_t count        = 0;
    uint32_t affix        = 0;
    uint32_t affixScore   = 0;
    uint32_t gem1         = 0;
    uint32_t gem2         = 0;
    uint32_t gem3         = 0;
    uint32_t gem4         = 0;
    uint32_t enchantLevel = 0;
    uint8_t  soulbound    = 0;
    uint32_t durability   = 0;
};

// Spell data (from SMSG_SPELLBOOK)
struct PlayerSpell
{
    uint32_t spellId = 0;
    uint32_t level   = 0;
};

struct SpellCooldown
{
    uint32_t spellId       = 0;
    int32_t  remainingSecs = 0;
};

// Quest data (from SMSG_QUEST_LIST)
struct QuestEntry
{
    uint32_t questId  = 0;
    uint8_t  rewarded = 0;
    uint32_t objectiveCount[MAX_QUEST_OBJECTIVES] = {};
};

// Local player data (populated from SMSG_PLAYER full snapshot)
struct LocalPlayerData
{
    uint32_t    guid       = 0;
    std::string name;
    uint8_t     classId    = 0;
    uint8_t     gender     = 0;
    uint8_t     portrait   = 0;
    uint16_t    level      = 0;
    int32_t     health     = 0;
    int32_t     maxHealth  = 0;
    int32_t     mana       = 0;
    int32_t     maxMana    = 0;
    uint32_t    xp         = 0;
    uint32_t    money      = 0;
    float       posX       = 0.f;
    float       posY       = 0.f;
    float       orientation = 0.f;
    uint32_t    mapId      = 0;
    int32_t     stats[STAT_TYPE_COUNT] = {};
    int32_t     armor      = 0;
    float       moveSpeed  = 0.f;
    int32_t     pvpPoints  = 0;
    int32_t     pkCount    = 0;
    uint8_t     pvpFlag    = 0;
    uint32_t    combatRating = 0;
    uint32_t    progression  = 0;
    uint8_t     isGM       = 0;

    // Inventory/bank/spells/quests populated by subsequent packets
    ItemInstance inventory[MAX_INVENTORY_SLOTS] = {};
    ItemInstance bank[MAX_BANK_SLOTS] = {};
    std::vector<PlayerSpell>   spells;
    std::vector<SpellCooldown> cooldowns;
    std::vector<QuestEntry>    quests;
};

class PacketHandler
{
public:
    static PacketHandler& instance();

    // Process all received packets from the connection.
    // Call each frame after Connection::update().
    void processPackets(std::vector<ReceivedPacket>& packets);

    // Auth state
    enum class AuthState
    {
        None,
        WaitingForResponse,
        Authenticated,
        Failed
    };

    AuthState getAuthState() const { return m_authState; }
    void setAuthState(AuthState state) { m_authState = state; }

    // Game state
    GameState getGameState() const { return m_gameState; }
    void setGameState(GameState state) { m_gameState = state; }

    // Character list (populated by SMSG_CharacterList)
    const std::vector<CharacterDefines::CharacterInfo>& getCharacterList() const { return m_characterList; }

    // Local player data (populated by SMSG_Player + subsequent packets)
    const LocalPlayerData& getLocalPlayer() const { return m_localPlayer; }
    LocalPlayerData& getLocalPlayer() { return m_localPlayer; }

    // Selected character guid (set before sending CMSG_EnterWorld)
    void setSelectedCharGuid(uint32_t guid) { m_selectedCharGuid = guid; }
    uint32_t getSelectedCharGuid() const { return m_selectedCharGuid; }

    // Last char create result
    bool wasCharCreateSuccess() const { return m_charCreateSuccess; }
    const std::string& getCharCreateMessage() const { return m_charCreateMessage; }

    // Phase 5: Chat message callback
    using ChatCallback = std::function<void(const std::string&)>;
    void setChatCallback(ChatCallback cb) { m_chatCallback = std::move(cb); }

private:
    PacketHandler() = default;

    // Dispatch a single packet by opcode.
    // Binary: processPacket at 0x00459b80
    void dispatch(uint16_t opcode, const uint8_t* data, size_t size);

    // Phase 1 handlers
    void handleValidate(StlBuffer& buf);       // 0x01 - auth response

    // Phase 2 handlers
    void handleCharacterList(StlBuffer& buf);  // 0x03 - character list
    void handlePlayer(StlBuffer& buf);         // 0x04 - player snapshot
    void handleInventory(StlBuffer& buf);      // 0x0B - inventory data
    void handleBank(StlBuffer& buf);           // 0x0C - bank data
    void handleQuestList(StlBuffer& buf);      // 0x13 - quest log
    void handleSpellbook(StlBuffer& buf);      // 0x50 - spellbook + cooldowns
    void handleCharCreate(StlBuffer& buf);     // 0x51 - char create response

    // Phase 4 handlers
    void handleUnit(StlBuffer& buf);               // 0x05 - NPC spawn
    void handleGameObject(StlBuffer& buf);          // 0x0A - GameObject spawn
    void handleUnitSpline(StlBuffer& buf);          // 0x07 - Spline movement
    void handleGameStateUpdate1(StlBuffer& buf);    // 0x4B - Position broadcast
    void handleObjectVariable(StlBuffer& buf);      // 0x52 - Variable update
    void handleChatMsg(StlBuffer& buf);             // 0x11 - Chat message

    AuthState m_authState   = AuthState::None;
    GameState m_gameState   = GameState::Login;

    // Character selection
    std::vector<CharacterDefines::CharacterInfo> m_characterList;
    uint32_t m_selectedCharGuid = 0;

    // Character creation result
    bool        m_charCreateSuccess = false;
    std::string m_charCreateMessage;

    // Local player (the character we're playing)
    LocalPlayerData m_localPlayer;

    // Phase 5: Chat callback
    ChatCallback m_chatCallback;
};

#define sPacketHandler PacketHandler::instance()
