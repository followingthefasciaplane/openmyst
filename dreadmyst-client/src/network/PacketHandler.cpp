#include "PacketHandler.h"
#include "PacketSender.h"
#include "game/WorldState.h"
#include "core/Log.h"
#include <cstring>

// PacketHandler implementation.
//
// Binary: processPacket at FUN_00459b80 (0x00459b80)
//   - 74 opcode cases in the original binary (range 0x01-0x97)
//   - Phase 1: SMSG_Validate (auth response)
//   - Phase 2: Character list, char create, player, inventory, spellbook,
//              quest list, bank
//
// Wire format matches reimplemented server's PacketBuffer:
//   - Strings: uint16 length + raw bytes (NOT null-terminated)
//   - Integers: little-endian
//   - sf::Packet handles framing (4-byte BE size header)

PacketHandler& PacketHandler::instance()
{
    static PacketHandler inst;
    return inst;
}

void PacketHandler::processPackets(std::vector<ReceivedPacket>& packets)
{
    for (auto& pkt : packets)
    {
        dispatch(pkt.opcode, pkt.payload.data(), pkt.payload.size());
    }
    packets.clear();
}

void PacketHandler::dispatch(uint16_t opcode, const uint8_t* data, size_t size)
{
    switch (opcode)
    {
        case WireOpcode::SMSG_Validate:
        {
            StlBuffer buf(data, size, false);
            handleValidate(buf);
            break;
        }
        case WireOpcode::SMSG_CharacterList:
        {
            StlBuffer buf(data, size, false);
            handleCharacterList(buf);
            break;
        }
        case WireOpcode::SMSG_Player:
        {
            StlBuffer buf(data, size, false);
            handlePlayer(buf);
            break;
        }
        case WireOpcode::SMSG_Inventory:
        {
            StlBuffer buf(data, size, false);
            handleInventory(buf);
            break;
        }
        case WireOpcode::SMSG_Bank:
        {
            StlBuffer buf(data, size, false);
            handleBank(buf);
            break;
        }
        case WireOpcode::SMSG_QuestList:
        {
            StlBuffer buf(data, size, false);
            handleQuestList(buf);
            break;
        }
        case WireOpcode::SMSG_Spellbook:
        {
            StlBuffer buf(data, size, false);
            handleSpellbook(buf);
            break;
        }
        case WireOpcode::SMSG_CharCreate:
        {
            StlBuffer buf(data, size, false);
            handleCharCreate(buf);
            break;
        }
        case WireOpcode::SMSG_Unit:
        {
            StlBuffer buf(data, size, false);
            handleUnit(buf);
            break;
        }
        case WireOpcode::SMSG_GameObject:
        {
            StlBuffer buf(data, size, false);
            handleGameObject(buf);
            break;
        }
        case WireOpcode::SMSG_UnitSpline:
        {
            StlBuffer buf(data, size, false);
            handleUnitSpline(buf);
            break;
        }
        case WireOpcode::SMSG_GameStateUpdate1:
        {
            StlBuffer buf(data, size, false);
            handleGameStateUpdate1(buf);
            break;
        }
        case WireOpcode::SMSG_GameState:
        {
            StlBuffer buf(data, size, false);
            handleObjectVariable(buf);
            break;
        }
        case WireOpcode::SMSG_ChatMsg:
        {
            StlBuffer buf(data, size, false);
            handleChatMsg(buf);
            break;
        }
        default:
            LOG_WARN("Unhandled opcode 0x%04X (%u bytes)", opcode, (unsigned)size);
            break;
    }
}

// ============================================================================
//  Phase 1: Auth response
// ============================================================================

// Binary: FUN_0045a940 at 0x0045a940
// Original binary reads: uint32 authResult, uint64 sessionToken
// Reimplemented server sends: uint8 result (0=fail, 1=success)
void PacketHandler::handleValidate(StlBuffer& buf)
{
    uint8_t result = buf.readUint8();

    if (result == 1)
    {
        LOG_INFO("Authentication successful");
        m_authState = AuthState::Authenticated;

        // Transition to CharSelect and request character list
        m_gameState = GameState::CharSelect;
        PacketSender::sendRequestCharList();
    }
    else
    {
        LOG_ERROR("Authentication failed (result=%u)", result);
        m_authState = AuthState::Failed;
    }
}

// ============================================================================
//  Phase 2: Character list
// ============================================================================

// Server format (from handleRequestCharacterList in server PacketHandler.cpp):
//   [count:u8]
//   For each character:
//     [name:string][class:u32][level:u32][guid:u32][portrait:u32][gender:u32]
void PacketHandler::handleCharacterList(StlBuffer& buf)
{
    m_characterList.clear();

    uint8_t count = buf.readUint8();
    LOG_INFO("Received character list: %u characters", count);

    for (uint8_t i = 0; i < count; ++i)
    {
        CharacterDefines::CharacterInfo info;
        info.name      = buf.readLenString();
        info.classId   = static_cast<PlayerDefines::Classes>(buf.readUint32());
        info.level     = static_cast<int>(buf.readUint32());
        info.guid      = static_cast<int>(buf.readUint32());
        info.portrait  = static_cast<int>(buf.readUint32());
        info.gender    = static_cast<PlayerDefines::Gender>(buf.readUint32());

        LOG_INFO("  [%u] guid=%d name='%s' class=%s level=%d gender=%s",
                 i, info.guid, info.name.c_str(),
                 PlayerDefines::classToString(info.classId),
                 info.level,
                 PlayerDefines::genderToString(info.gender));

        m_characterList.push_back(std::move(info));
    }

    m_gameState = GameState::CharSelect;
}

// ============================================================================
//  Phase 2: Character creation response
// ============================================================================

// Server format (from handleCharCreate in server PacketHandler.cpp):
//   [success:u8][name_or_error:string]
void PacketHandler::handleCharCreate(StlBuffer& buf)
{
    uint8_t success = buf.readUint8();
    std::string message = buf.readLenString();

    m_charCreateSuccess = (success == 1);
    m_charCreateMessage = message;

    if (m_charCreateSuccess)
    {
        LOG_INFO("Character created: '%s'", message.c_str());
        // Request updated character list
        PacketSender::sendRequestCharList();
    }
    else
    {
        LOG_ERROR("Character creation failed: %s", message.c_str());
    }
}

// ============================================================================
//  Phase 2: Player data snapshot
// ============================================================================

// Server sends two formats, both using SMSG_PLAYER (0x04):
//
// Full snapshot (for the entering player - "self"):
//   [guid:u32][name:string][class:u8][gender:u8][portrait:u8][level:u16]
//   [hp:i32][maxhp:i32][mana:i32][maxmana:i32][xp:u32][money:u32]
//   [x:f32][y:f32][orientation:f32][mapid:u32]
//   [stats × 7 : i32 each][armor:i32][movespeed:f32]
//   [pvppoints:i32][pkcount:i32][pvpflag:u8]
//   [combatrating:u32][progression:u32][isgm:u8]
//
// Abbreviated (for other players on map):
//   [guid:u32][name:string][class:u8][gender:u8][portrait:u8][level:u16]
//   [hp:i32][maxhp:i32][mana:i32][maxmana:i32]
//   [x:f32][y:f32][orientation:f32]
//
// We distinguish by checking if the guid matches our selected character.
void PacketHandler::handlePlayer(StlBuffer& buf)
{
    uint32_t guid   = buf.readUint32();
    std::string name = buf.readLenString();
    uint8_t classId  = buf.readUint8();
    uint8_t gender   = buf.readUint8();
    uint8_t portrait = buf.readUint8();
    uint16_t level   = buf.readUint16();
    int32_t health   = buf.readInt32();
    int32_t maxHealth = buf.readInt32();
    int32_t mana     = buf.readInt32();
    int32_t maxMana  = buf.readInt32();

    bool isSelf = (guid == m_selectedCharGuid);

    if (isSelf)
    {
        // Full snapshot - read all remaining fields
        uint32_t xp    = buf.readUint32();
        uint32_t money = buf.readUint32();
        float posX     = buf.readFloat();
        float posY     = buf.readFloat();
        float orient   = buf.readFloat();
        uint32_t mapId = buf.readUint32();

        // 7 stat types
        int32_t stats[STAT_TYPE_COUNT];
        for (int i = 0; i < STAT_TYPE_COUNT; ++i)
            stats[i] = buf.readInt32();

        int32_t armor     = buf.readInt32();
        float moveSpeed   = buf.readFloat();
        int32_t pvpPoints = buf.readInt32();
        int32_t pkCount   = buf.readInt32();
        uint8_t pvpFlag   = buf.readUint8();
        uint32_t combatRating = buf.readUint32();
        uint32_t progression  = buf.readUint32();
        uint8_t isGM     = buf.readUint8();

        // Store in local player data
        m_localPlayer.guid        = guid;
        m_localPlayer.name        = name;
        m_localPlayer.classId     = classId;
        m_localPlayer.gender      = gender;
        m_localPlayer.portrait    = portrait;
        m_localPlayer.level       = level;
        m_localPlayer.health      = health;
        m_localPlayer.maxHealth   = maxHealth;
        m_localPlayer.mana        = mana;
        m_localPlayer.maxMana     = maxMana;
        m_localPlayer.xp          = xp;
        m_localPlayer.money       = money;
        m_localPlayer.posX        = posX;
        m_localPlayer.posY        = posY;
        m_localPlayer.orientation = orient;
        m_localPlayer.mapId       = mapId;
        for (int i = 0; i < STAT_TYPE_COUNT; ++i)
            m_localPlayer.stats[i] = stats[i];
        m_localPlayer.armor       = armor;
        m_localPlayer.moveSpeed   = moveSpeed;
        m_localPlayer.pvpPoints   = pvpPoints;
        m_localPlayer.pkCount     = pkCount;
        m_localPlayer.pvpFlag     = pvpFlag;
        m_localPlayer.combatRating = combatRating;
        m_localPlayer.progression = progression;
        m_localPlayer.isGM        = isGM;

        LOG_INFO("SMSG_PLAYER (self): guid=%u name='%s' class=%u level=%u map=%u pos=(%.1f,%.1f)",
                 guid, name.c_str(), classId, level, mapId, posX, posY);

        m_gameState = GameState::InWorld;
    }
    else
    {
        // Abbreviated snapshot for another player
        float posX   = buf.readFloat();
        float posY   = buf.readFloat();
        float orient = buf.readFloat();

        LOG_INFO("SMSG_PLAYER (other): guid=%u name='%s' class=%u level=%u pos=(%.1f,%.1f)",
                 guid, name.c_str(), classId, level, posX, posY);

        // Phase 4: Register in world entity registry
        WorldEntity& ent = sWorldState.addEntity(guid);
        ent.type       = ObjDefines::Type_Player;
        ent.name       = name;
        ent.classId    = classId;
        ent.gender     = gender;
        ent.portrait   = portrait;
        ent.level      = level;
        ent.health     = health;
        ent.maxHealth  = maxHealth;
        ent.mana       = mana;
        ent.maxMana    = maxMana;
        ent.posX       = posX;
        ent.posY       = posY;
        ent.targetX    = posX;
        ent.targetY    = posY;
        ent.orientation = orient;
    }
}

// ============================================================================
//  Phase 2: Inventory (40 slots)
// ============================================================================

// Server format (from handleEnterWorld):
//   For each of 40 slots:
//     [entry:u32]
//     if entry != 0:
//       [count:u32][affix:u32][affixScore:u32]
//       [gem1:u32][gem2:u32][gem3:u32][gem4:u32]
//       [enchantLevel:u32][soulbound:u8][durability:u32]
void PacketHandler::handleInventory(StlBuffer& buf)
{
    int itemCount = 0;
    for (int i = 0; i < MAX_INVENTORY_SLOTS; ++i)
    {
        ItemInstance& item = m_localPlayer.inventory[i];
        item = {};  // Reset

        item.entry = buf.readUint32();
        if (item.entry != 0)
        {
            item.count        = buf.readUint32();
            item.affix        = buf.readUint32();
            item.affixScore   = buf.readUint32();
            item.gem1         = buf.readUint32();
            item.gem2         = buf.readUint32();
            item.gem3         = buf.readUint32();
            item.gem4         = buf.readUint32();
            item.enchantLevel = buf.readUint32();
            item.soulbound    = buf.readUint8();
            item.durability   = buf.readUint32();
            itemCount++;
        }
    }

    LOG_INFO("SMSG_INVENTORY: %d items in %d slots", itemCount, MAX_INVENTORY_SLOTS);
}

// ============================================================================
//  Phase 2: Bank (40 slots, same format as inventory)
// ============================================================================

void PacketHandler::handleBank(StlBuffer& buf)
{
    int itemCount = 0;
    for (int i = 0; i < MAX_BANK_SLOTS; ++i)
    {
        ItemInstance& item = m_localPlayer.bank[i];
        item = {};

        item.entry = buf.readUint32();
        if (item.entry != 0)
        {
            item.count        = buf.readUint32();
            item.affix        = buf.readUint32();
            item.affixScore   = buf.readUint32();
            item.gem1         = buf.readUint32();
            item.gem2         = buf.readUint32();
            item.gem3         = buf.readUint32();
            item.gem4         = buf.readUint32();
            item.enchantLevel = buf.readUint32();
            item.soulbound    = buf.readUint8();
            item.durability   = buf.readUint32();
            itemCount++;
        }
    }

    LOG_INFO("SMSG_BANK: %d items in %d slots", itemCount, MAX_BANK_SLOTS);
}

// ============================================================================
//  Phase 2: Spellbook + cooldowns
// ============================================================================

// Server format (from handleEnterWorld):
//   [numSpells:u16]
//   For each spell: [spellId:u32][level:u32]
//   [numCooldowns:u16]
//   For each cooldown: [spellId:u32][remainingSecs:i32]
void PacketHandler::handleSpellbook(StlBuffer& buf)
{
    m_localPlayer.spells.clear();
    m_localPlayer.cooldowns.clear();

    uint16_t numSpells = buf.readUint16();
    for (uint16_t i = 0; i < numSpells; ++i)
    {
        PlayerSpell spell;
        spell.spellId = buf.readUint32();
        spell.level   = buf.readUint32();
        m_localPlayer.spells.push_back(spell);
    }

    uint16_t numCooldowns = buf.readUint16();
    for (uint16_t i = 0; i < numCooldowns; ++i)
    {
        SpellCooldown cd;
        cd.spellId       = buf.readUint32();
        cd.remainingSecs = buf.readInt32();
        m_localPlayer.cooldowns.push_back(cd);
    }

    LOG_INFO("SMSG_SPELLBOOK: %u spells, %u cooldowns", numSpells, numCooldowns);
}

// ============================================================================
//  Phase 2: Quest list
// ============================================================================

// Server format (from handleEnterWorld):
//   [numQuests:u16]
//   For each quest:
//     [questId:u32][rewarded:u8][obj0:u32][obj1:u32][obj2:u32][obj3:u32]
void PacketHandler::handleQuestList(StlBuffer& buf)
{
    m_localPlayer.quests.clear();

    uint16_t numQuests = buf.readUint16();
    for (uint16_t i = 0; i < numQuests; ++i)
    {
        QuestEntry quest;
        quest.questId  = buf.readUint32();
        quest.rewarded = buf.readUint8();
        for (int j = 0; j < MAX_QUEST_OBJECTIVES; ++j)
            quest.objectiveCount[j] = buf.readUint32();
        m_localPlayer.quests.push_back(quest);
    }

    LOG_INFO("SMSG_QUEST_LIST: %u quests", numQuests);
}

// ============================================================================
//  Phase 4: NPC spawn (SMSG_Unit = 0x05)
// ============================================================================

// Binary: FUN_00455280 + FUN_00455010
// Format: [guid:u32][posX:i32->float][posY:i32->float]
//         [numVars:u32][varId:u8, value:i32 × numVars][entryId:u32]
void PacketHandler::handleUnit(StlBuffer& buf)
{
    uint32_t guid = buf.readUint32();

    // Positions stored as int32 but reinterpreted as float
    int32_t rawX = buf.readInt32();
    int32_t rawY = buf.readInt32();
    float posX, posY;
    std::memcpy(&posX, &rawX, sizeof(float));
    std::memcpy(&posY, &rawY, sizeof(float));

    uint32_t numVars = buf.readUint32();

    WorldEntity& ent = sWorldState.addEntity(guid);
    ent.type = ObjDefines::Type_Npc;
    ent.posX = posX;
    ent.posY = posY;
    ent.targetX = posX;
    ent.targetY = posY;

    for (uint32_t i = 0; i < numVars; ++i)
    {
        uint8_t varId = buf.readUint8();
        int32_t value = buf.readInt32();
        ent.setVariable(varId, value);
    }

    ent.entryId = buf.readUint32();

    // Extract common vars for convenience
    ent.health    = ent.getVariable(ObjDefines::Health);
    ent.maxHealth = ent.getVariable(ObjDefines::MaxHealth);
    ent.mana      = ent.getVariable(ObjDefines::Mana);
    ent.maxMana   = ent.getVariable(ObjDefines::MaxMana);
    ent.level     = static_cast<uint16_t>(ent.getVariable(ObjDefines::Level));

    LOG_INFO("SMSG_UNIT: guid=%u entry=%u pos=(%.1f,%.1f) vars=%u",
             guid, ent.entryId, posX, posY, numVars);
}

// ============================================================================
//  Phase 4: GameObject spawn (SMSG_GameObject = 0x0A)
// ============================================================================

// Binary: FUN_00455010
// Same as Unit but without entryId at the end.
void PacketHandler::handleGameObject(StlBuffer& buf)
{
    uint32_t guid = buf.readUint32();

    int32_t rawX = buf.readInt32();
    int32_t rawY = buf.readInt32();
    float posX, posY;
    std::memcpy(&posX, &rawX, sizeof(float));
    std::memcpy(&posY, &rawY, sizeof(float));

    uint32_t numVars = buf.readUint32();

    WorldEntity& ent = sWorldState.addEntity(guid);
    ent.type = ObjDefines::Type_GameObject;
    ent.posX = posX;
    ent.posY = posY;
    ent.targetX = posX;
    ent.targetY = posY;

    for (uint32_t i = 0; i < numVars; ++i)
    {
        uint8_t varId = buf.readUint8();
        int32_t value = buf.readInt32();
        ent.setVariable(varId, value);
    }

    LOG_INFO("SMSG_GAMEOBJECT: guid=%u pos=(%.1f,%.1f) vars=%u",
             guid, posX, posY, numVars);
}

// ============================================================================
//  Phase 4: Unit spline movement (SMSG_UnitSpline = 0x07)
// ============================================================================

// Binary: FUN_004530f0
// Format: [guid:u32][isMoving:u8][startX:f32][startY:f32]
//         [hasWaypoints:u8][waypointCount:u16][{x:f32,y:f32} × count]
void PacketHandler::handleUnitSpline(StlBuffer& buf)
{
    uint32_t guid    = buf.readUint32();
    uint8_t moving   = buf.readUint8();
    float startX     = buf.readFloat();
    float startY     = buf.readFloat();
    uint8_t hasWPs   = buf.readUint8();
    uint16_t wpCount = buf.readUint16();

    WorldEntity* ent = sWorldState.getEntity(guid);
    if (!ent)
    {
        LOG_WARN("SMSG_UNITSPLINE: unknown entity guid=%u", guid);
        // Skip remaining data
        for (uint16_t i = 0; i < wpCount; ++i)
        {
            buf.readFloat();
            buf.readFloat();
        }
        return;
    }

    ent->splineStartX = startX;
    ent->splineStartY = startY;
    ent->splineWaypoints.clear();
    ent->splineIndex = 0;

    if (moving && hasWPs && wpCount > 0)
    {
        ent->splineWaypoints.reserve(wpCount);
        for (uint16_t i = 0; i < wpCount; ++i)
        {
            WorldEntity::Waypoint wp;
            wp.x = buf.readFloat();
            wp.y = buf.readFloat();
            ent->splineWaypoints.push_back(wp);
        }
        ent->splineActive = true;
        ent->isMoving = true;
    }
    else
    {
        for (uint16_t i = 0; i < wpCount; ++i)
        {
            buf.readFloat();
            buf.readFloat();
        }
        ent->splineActive = false;
        ent->isMoving = false;
    }

    LOG_INFO("SMSG_UNITSPLINE: guid=%u moving=%u waypoints=%u",
             guid, moving, wpCount);
}

// ============================================================================
//  Phase 4: Position broadcast (SMSG_GameStateUpdate1 = 0x4B)
// ============================================================================

// Format: [guid:u32][posX:f32][posY:f32][orientation:f32]
void PacketHandler::handleGameStateUpdate1(StlBuffer& buf)
{
    uint32_t guid = buf.readUint32();
    float posX    = buf.readFloat();
    float posY    = buf.readFloat();
    float orient  = buf.readFloat();

    // Update local player position if it's us
    if (guid == m_localPlayer.guid)
    {
        m_localPlayer.posX = posX;
        m_localPlayer.posY = posY;
        m_localPlayer.orientation = orient;
        return;
    }

    // Update world entity
    WorldEntity* ent = sWorldState.getEntity(guid);
    if (ent)
    {
        ent->targetX = posX;
        ent->targetY = posY;
        ent->orientation = orient;
        // Don't snap position - interpolation in WorldState::update() handles it
    }
}

// ============================================================================
//  Phase 4: ObjectVariable update (SMSG_GameState = 0x52)
// ============================================================================

// Format: [guid:u32][varId:u8][value:i32]
void PacketHandler::handleObjectVariable(StlBuffer& buf)
{
    uint32_t guid  = buf.readUint32();
    uint8_t varId  = buf.readUint8();
    int32_t value  = buf.readInt32();

    // Update local player if it's us
    if (guid == m_localPlayer.guid)
    {
        switch (varId)
        {
            case ObjDefines::Health:     m_localPlayer.health    = value; break;
            case ObjDefines::MaxHealth:  m_localPlayer.maxHealth = value; break;
            case ObjDefines::Mana:       m_localPlayer.mana      = value; break;
            case ObjDefines::MaxMana:    m_localPlayer.maxMana   = value; break;
            case ObjDefines::Level:      m_localPlayer.level     = static_cast<uint16_t>(value); break;
            case ObjDefines::Experience: m_localPlayer.xp        = static_cast<uint32_t>(value); break;
            case ObjDefines::Gold:       m_localPlayer.money     = static_cast<uint32_t>(value); break;
            case ObjDefines::MoveSpeed:  m_localPlayer.moveSpeed = *reinterpret_cast<float*>(&value); break;
            default: break;
        }
        return;
    }

    // Update world entity
    WorldEntity* ent = sWorldState.getEntity(guid);
    if (ent)
    {
        ent->setVariable(varId, value);

        // Update convenience fields
        switch (varId)
        {
            case ObjDefines::Health:    ent->health    = value; break;
            case ObjDefines::MaxHealth: ent->maxHealth = value; break;
            case ObjDefines::Mana:      ent->mana      = value; break;
            case ObjDefines::MaxMana:   ent->maxMana   = value; break;
            case ObjDefines::Level:     ent->level     = static_cast<uint16_t>(value); break;
            default: break;
        }
    }
}

// ============================================================================
//  Phase 4: Chat message (SMSG_ChatMsg = 0x11)
// ============================================================================

// Format: [channel:u8][senderName:string][message:string]
void PacketHandler::handleChatMsg(StlBuffer& buf)
{
    uint8_t channel     = buf.readUint8();
    std::string sender  = buf.readLenString();
    std::string message = buf.readLenString();

    const char* channelName = "Unknown";
    switch (channel)
    {
        case 0: channelName = "Say";     break;
        case 1: channelName = "Whisper"; break;
        case 2: channelName = "Party";   break;
        case 3: channelName = "Guild";   break;
        case 4: channelName = "AllChat"; break;
        case 5: channelName = "System";  break;
    }

    LOG_INFO("[%s] %s: %s", channelName, sender.c_str(), message.c_str());

    // Phase 5: Forward to UI
    if (m_chatCallback)
    {
        std::string formatted = std::string("[") + channelName + "] " + sender + ": " + message;
        m_chatCallback(formatted);
    }
}
