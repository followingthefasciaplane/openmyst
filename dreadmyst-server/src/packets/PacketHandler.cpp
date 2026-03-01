#include "packets/PacketHandler.h"
#include "core/Types.h"
#include "core/Log.h"
#include "core/Server.h"
#include "network/Session.h"
#include "network/GamePacket.h"
#include "network/Opcodes.h"
#include "database/DatabaseMgr.h"
#include "database/PlayerDb.h"
#include "game/ServerPlayer.h"
#include "game/ServerNpc.h"
#include "game/ServerMap.h"
#include "game/ServerGameObj.h"
#include "game/GameCache.h"
#include "game/PlayerMgr.h"
#include "game/ChatSystem.h"
#include "game/Spell.h"
#include "game/Guild.h"
#include "game/Party.h"
#include "game/Arena.h"
#include "game/Trade.h"

#include <string>
#include <ctime>
#include <cstring>
#include <memory>

// ============================================================================
//  Helper: retrieve the ServerPlayer attached to a session, or nullptr
// ============================================================================
static ServerPlayer* getPlayer(Session* session) {
    if (!session)
        return nullptr;
    Guid guid = session->getPlayerGuid();
    if (guid == 0)
        return nullptr;
    return PlayerMgr::instance().findPlayer(guid);
}

// ============================================================================
//  Helper: check that the session is authenticated (state >= Authenticated)
// ============================================================================
static bool requireAuth(Session* session) {
    if (!session || !session->isAuthenticated()) {
        LOG_WARN("PacketHandler: received packet from unauthenticated session");
        return false;
    }
    return true;
}

// ============================================================================
//  Helper: check that the session has a player in-world
// ============================================================================
static bool requireInWorld(Session* session) {
    if (!requireAuth(session))
        return false;
    if (session->getPlayerGuid() == 0) {
        LOG_WARN("PacketHandler: received in-world packet but player not loaded");
        return false;
    }
    return true;
}

// ============================================================================
//  Master dispatch (binary: 0x00489310)
//
//  Reads uint16 opcode, subtracts 1, validates <= 0x96 (150), then switches.
//  Each case maps to a handler function.
// ============================================================================
bool PacketHandler::dispatch(Session* session, uint16 opcode, GamePacket& packet) {
    if (opcode == 0 || opcode > MAX_OPCODE) {
        LOG_WARN("received bad opcode %d", opcode);
        return false;
    }

    // The binary dispatches on (opcode - 1)
    uint16 index = opcode - 1;

    switch (index) {
        case 0x00: // opcode 1 - Time sync
            handleTimeSync(session, packet);
            break;
        case 0x01: // opcode 2 - Authenticate
            handleAuthenticate(session, packet);
            break;
        // case 0x02 (opcode 3) - not mapped in binary
        case 0x03: // opcode 4 - Character create
            handleCharCreate(session, packet);
            break;
        case 0x04: // opcode 5 - Enter world
            handleEnterWorld(session, packet);
            break;
        case 0x05: // opcode 6 - Request character list
            handleRequestCharList(session, packet);
            break;
        case 0x06: // opcode 7 - Delete character
            handleDeleteChar(session, packet);
            break;
        case 0x07: // opcode 8 - Chat message
            handleChatMessage(session, packet);
            break;
        case 0x08: // opcode 9 - Movement update
            handleMovement(session, packet);
            break;
        case 0x09: // opcode 10 - Movement update type 2
            handleMovement(session, packet);
            break;
        case 0x0A: // opcode 11 - Target change
            handleTargetChange(session, packet);
            break;
        case 0x0B: // opcode 12 - Inventory swap
            handleInventorySwap(session, packet);
            break;
        case 0x0C: // opcode 13 - Equipment swap
            handleEquipmentSwap(session, packet);
            break;
        case 0x0D: // opcode 14 - Use item
            handleUseItem(session, packet);
            break;
        case 0x0E: // opcode 15 - Bank deposit/withdraw
            handleBankDeposit(session, packet);
            break;
        case 0x0F: // opcode 16 - Bank swap
            handleBankSwap(session, packet);
            break;
        case 0x10: // opcode 17 - Vendor buy
            handleVendorBuy(session, packet);
            break;
        case 0x11: // opcode 18 - Guild MOTD
            handleGuildMotd(session, packet);
            break;
        case 0x12: // opcode 19 - Chat msg type 2
            handleChatMessage(session, packet);
            break;
        case 0x13: // opcode 20 - NPC interact / gossip
            handleNpcInteract(session, packet);
            break;
        case 0x14: // opcode 21 - Request player info/inspect
            handleInspect(session, packet);
            break;
        case 0x15: // opcode 22 - Quest accept/complete
            handleQuestAction(session, packet);
            break;
        case 0x16: // opcode 23 - Guild create
            handleGuildCreate(session, packet);
            break;
        case 0x17: // opcode 24 - Guild invite
            handleGuildInvite(session, packet);
            break;
        case 0x18: // opcode 25 - Party invite
            handlePartyInvite(session, packet);
            break;
        case 0x19: // opcode 26 - Party accept/decline
            handlePartyResponse(session, packet);
            break;
        case 0x1A: // opcode 27 - Cast spell
            handleCastSpell(session, packet);
            break;
        case 0x1B: // opcode 28 - Auto attack toggle
            handleAutoAttack(session, packet);
            break;
        case 0x1C: // opcode 29 - Set target
            handleSetTarget(session, packet);
            break;
        case 0x1D: // opcode 30 - Loot action
            handleLootAction(session, packet);
            break;
        case 0x1E: // opcode 31 - Trade request
            handleTradeRequest(session, packet);
            break;
        case 0x1F: // opcode 32 - Quest NPC interact
            handleNpcInteract(session, packet);
            break;
        case 0x20: // opcode 33 - Emote/action
            LOG_DEBUG("Unhandled opcode %d (Emote/action)", opcode);
            break;
        case 0x21: // opcode 34 - Request waypoints
            LOG_DEBUG("Unhandled opcode %d (Request waypoints)", opcode);
            break;
        case 0x22: // opcode 35 - Movement spline
            handleMovement(session, packet);
            break;
        case 0x23: // opcode 36 - Request something
            LOG_DEBUG("Unhandled opcode %d (Request)", opcode);
            break;
        case 0x24: // opcode 37 - Ignore player
            LOG_DEBUG("Unhandled opcode %d (Ignore player)", opcode);
            break;
        case 0x25: // opcode 38 - Request stats
            LOG_DEBUG("Unhandled opcode %d (Request stats)", opcode);
            break;
        case 0x26: // opcode 39 - Report player
            LOG_DEBUG("Unhandled opcode %d (Report player)", opcode);
            break;
        case 0x27: // opcode 40 - Modify/kick GM action
            LOG_DEBUG("Unhandled opcode %d (GM kick)", opcode);
            break;
        case 0x28: // opcode 41 - Ban GM action
            LOG_DEBUG("Unhandled opcode %d (GM ban)", opcode);
            break;
        case 0x29: // opcode 42 - Level up request
            LOG_DEBUG("Unhandled opcode %d (Level up request)", opcode);
            break;
        case 0x2A: // opcode 43 - Auto attack check
            handleAutoAttack(session, packet);
            break;
        case 0x2B: // opcode 44 - Inventory update all
            LOG_DEBUG("Unhandled opcode %d (Inventory update all)", opcode);
            break;
        case 0x2C: // opcode 45 - Combine items (crafting)
            handleCombineItems(session, packet);
            break;
        case 0x2D: // opcode 46 - Trade action
            handleTradeRequest(session, packet);
            break;
        case 0x2E: // opcode 47 - Stat invest
            handleStatInvest(session, packet);
            break;
        case 0x2F: // opcode 48 - Quest update
            handleQuestAction(session, packet);
            break;
        case 0x30: // opcode 49 - Quest objective progress
            handleQuestAction(session, packet);
            break;
        case 0x31: // opcode 50 - Request NPC info
            handleNpcInteract(session, packet);
            break;
        case 0x32: // opcode 51 - Request game object info
            handleGameObjInteract(session, packet);
            break;
        case 0x33: // opcode 52 - Game object interact
            handleGameObjInteract(session, packet);
            break;
        case 0x34: // opcode 53 - Spell cancel
            handleSpellCancel(session, packet);
            break;
        case 0x35: // opcode 54 - Gossip option select
            handleGossipSelect(session, packet);
            break;
        case 0x36: // opcode 55 - Arena queue (solo)
            handleArenaQueue(session, packet);
            break;
        case 0x37: // opcode 56 - Arena queue (party)
            handleArenaQueue(session, packet);
            break;
        case 0x38: // opcode 57 - Duel request
            LOG_DEBUG("Unhandled opcode %d (Duel request)", opcode);
            break;
        case 0x39: // opcode 58 - Waypoint teleport
            handleWaypointTeleport(session, packet);
            break;
        case 0x3A: // opcode 59 - Guild invite response
            handleGuildInviteResponse(session, packet);
            break;
        case 0x3B: // opcode 60 - Guild leave
            handleGuildLeave(session, packet);
            break;
        case 0x3C: // opcode 61 - Guild kick
            handleGuildKick(session, packet);
            break;
        case 0x3D: // opcode 62 - Guild rank change
            handleGuildRankChange(session, packet);
            break;
        case 0x3E: // opcode 63 - Party changes
            handlePartyChanges(session, packet);
            break;
        case 0x3F: // opcode 64 - Party leave
            handlePartyLeave(session, packet);
            break;
        case 0x40: // opcode 65 - Mail send
            handleMailSend(session, packet);
            break;
        case 0x41: // opcode 66 - Mail action
            handleMailAction(session, packet);
            break;
        case 0x42: // opcode 67 - Arena action / queue cancel
            handleArenaQueue(session, packet);
            break;
        case 0x43: // opcode 68 - Mod warn
            LOG_DEBUG("Unhandled opcode %d (Mod warn)", opcode);
            break;
        case 0x44: // opcode 69 - Mod mute
            LOG_DEBUG("Unhandled opcode %d (Mod mute)", opcode);
            break;
        case 0x45: // opcode 70 - Inspect request
            handleInspect(session, packet);
            break;
        case 0x46: // opcode 71 - Some action
            LOG_DEBUG("Unhandled opcode %d", opcode);
            break;
        case 0x47: // opcode 72 - Some action
            LOG_DEBUG("Unhandled opcode %d", opcode);
            break;
        case 0x48: // opcode 73 - Open trade
            handleTradeRequest(session, packet);
            break;
        case 0x49: // opcode 74 - Trade finalize
            handleTradeRequest(session, packet);
            break;
        case 0x4A: // opcode 75 - Login with credentials
            handleLogin(session, packet);
            break;
        default:
            LOG_DEBUG("received bad opcode %d", opcode);
            return false;
    }

    return true;
}

// ============================================================================
//  Opcode 1 - Time sync
//  Reads int64 timestamp from client, sends server timestamp back.
// ============================================================================
void PacketHandler::handleTimeSync(Session* session, GamePacket& packet) {
    // Client sends int64 timestamp (two int32s in the binary)
    int32 timeLow  = packet.readInt32();
    int32 timeHigh = packet.readInt32();
    (void)timeLow;
    (void)timeHigh;

    // Respond with server time
    GamePacket response(SMSG_GAME_STATE);
    response.writeInt32(static_cast<int32>(Server::instance().getUnixTime() & 0xFFFFFFFF));
    response.writeInt32(static_cast<int32>(Server::instance().getUnixTime() >> 32));
    session->send(response);
}

// ============================================================================
//  Opcode 2 - Authenticate
//  Binary SQL: "SELECT user_id, token FROM auth_tokens WHERE token = '%s'
//               AND created_at > NOW() - INTERVAL 60 SECOND"
// ============================================================================
void PacketHandler::handleAuthenticate(Session* session, GamePacket& packet) {
    std::string token = packet.readString();
    uint32 version    = packet.readUint32();

    LOG_INFO("Auth request: token=%s version=%u", token.c_str(), version);

    // Validate client version
    if (version != DREADMYST_REVISION) {
        LOG_WARN("Client version mismatch: got %u, expected %d", version, DREADMYST_REVISION);
        GamePacket response(SMSG_VALIDATE);
        response.writeUint8(0); // failure
        session->send(response);
        session->disconnect();
        return;
    }

    // Validate auth token against database
    PlayerDb playerDb;
    playerDb.setConnector(&DatabaseMgr::instance().getPlayerConnection());

    DbAuthToken authResult;
    if (!playerDb.validateAuthToken(token, authResult) || !authResult.valid) {
        LOG_WARN("Auth failed for token: %s", token.c_str());
        GamePacket response(SMSG_VALIDATE);
        response.writeUint8(0); // failure
        session->send(response);
        session->disconnect();
        return;
    }

    // Check for duplicate login
    ServerPlayer* existing = PlayerMgr::instance().findPlayerByAccount(authResult.userId);
    if (existing) {
        LOG_WARN("Duplicate login for account %u, disconnecting old session", authResult.userId);
        Session* oldSession = existing->getSession();
        if (oldSession) {
            oldSession->disconnect();
        }
    }

    // Set session account and transition to Authenticated
    session->setAccountId(authResult.userId);
    session->setState(SessionState::Authenticated);
    session->clearTimeout();

    LOG_INFO("Account %u authenticated successfully", authResult.userId);

    // Send validation success
    GamePacket response(SMSG_VALIDATE);
    response.writeUint8(1); // success
    session->send(response);
}

// ============================================================================
//  Opcode 75 - Login with username+password
//  Validates credentials against the accounts table directly.
//  Bypasses the auth_tokens table entirely.
// ============================================================================
void PacketHandler::handleLogin(Session* session, GamePacket& packet) {
    std::string username = packet.readString();
    std::string password = packet.readString();
    uint32 version       = packet.readUint32();

    LOG_INFO("Login request: user=%s version=%u", username.c_str(), version);

    // Validate client version
    if (version != DREADMYST_REVISION) {
        LOG_WARN("Login version mismatch: got %u, expected %d", version, DREADMYST_REVISION);
        GamePacket response(SMSG_VALIDATE);
        response.writeUint8(0);
        session->send(response);
        session->disconnect();
        return;
    }

    // Validate credentials against accounts table
    PlayerDb playerDb;
    playerDb.setConnector(&DatabaseMgr::instance().getPlayerConnection());

    uint32 accountId = 0;
    if (!playerDb.validateLogin(username, password, accountId)) {
        LOG_WARN("Login failed for user: %s", username.c_str());
        GamePacket response(SMSG_VALIDATE);
        response.writeUint8(0);
        session->send(response);
        session->disconnect();
        return;
    }

    // Check for duplicate login
    ServerPlayer* existing = PlayerMgr::instance().findPlayerByAccount(accountId);
    if (existing) {
        LOG_WARN("Duplicate login for account %u, disconnecting old session", accountId);
        Session* oldSession = existing->getSession();
        if (oldSession)
            oldSession->disconnect();
    }

    // Set session authenticated
    session->setAccountId(accountId);
    session->setState(SessionState::Authenticated);
    session->clearTimeout();

    LOG_INFO("Account %u (%s) logged in successfully", accountId, username.c_str());

    GamePacket response(SMSG_VALIDATE);
    response.writeUint8(1); // success
    session->send(response);
}

// ============================================================================
//  Opcode 6 - Request character list
//  SQL: "SELECT name, class, level, guid, portrait, gender
//        FROM player WHERE account = '%d'"
// ============================================================================
void PacketHandler::handleRequestCharList(Session* session, GamePacket& packet) {
    if (!requireAuth(session))
        return;

    uint32 accountId = session->getAccountId();

    PlayerDb playerDb;
    playerDb.setConnector(&DatabaseMgr::instance().getPlayerConnection());

    std::vector<DbCharacterListEntry> charList;
    if (!playerDb.getCharacterList(accountId, charList)) {
        LOG_ERROR("Failed to load character list for account %u", accountId);
        return;
    }

    // Build SMSG_CHARACTER_LIST
    GamePacket response(SMSG_CHARACTER_LIST);
    response.writeUint8(static_cast<uint8>(charList.size()));

    for (const auto& entry : charList) {
        response.writeString(entry.name);
        response.writeUint32(entry.playerClass);
        response.writeUint32(entry.level);
        response.writeUint32(entry.guid);
        response.writeUint32(entry.portrait);
        response.writeUint32(entry.gender);
    }

    session->send(response);
}

// ============================================================================
//  Opcode 4 - Character create
//  Reads name (string), classId (uint8), gender (uint8), portrait (uint8).
//  Name must be <= 19 characters.
// ============================================================================
void PacketHandler::handleCharCreate(Session* session, GamePacket& packet) {
    if (!requireAuth(session))
        return;

    std::string name  = packet.readString();
    uint8 classId     = packet.readUint8();
    uint8 gender      = packet.readUint8();
    uint8 portrait    = packet.readUint8();

    // Validate name length
    if (name.empty() || name.length() > 19) {
        LOG_WARN("CharCreate: invalid name length %zu", name.length());
        GamePacket response(SMSG_CHAR_CREATE);
        response.writeUint8(0); // failure
        response.writeString("Invalid character name.");
        session->send(response);
        return;
    }

    PlayerDb playerDb;
    playerDb.setConnector(&DatabaseMgr::instance().getPlayerConnection());

    // Check if name is taken
    bool taken = false;
    if (!playerDb.isNameTaken(name, taken) || taken) {
        LOG_WARN("CharCreate: name '%s' is already taken", name.c_str());
        GamePacket response(SMSG_CHAR_CREATE);
        response.writeUint8(0); // failure
        response.writeString("That name is already taken.");
        session->send(response);
        return;
    }

    // Get next GUID from GameCache
    GameCache* cache = Server::instance().getGameCache();
    uint32 newGuid = cache->getMaxPlayerGuid() + 1;
    cache->setMaxPlayerGuid(newGuid);

    // Get starting position from the first map template
    const MapTemplate* startMap = cache->getMapTemplate(1); // default starting map
    float startX = 0.0f;
    float startY = 0.0f;
    uint32 startMapId = 1;
    if (startMap) {
        startX = startMap->startX;
        startY = startMap->startY;
        startMapId = startMap->id;
    }

    // Insert into database
    // The binary builds an INSERT query with all initial player fields
    MySQLConnector& db = DatabaseMgr::instance().getPlayerConnection();
    char query[1024];
    snprintf(query, sizeof(query),
        "INSERT INTO player (guid, account, name, class, gender, portrait, level, "
        "hp_pct, mp_pct, xp, money, position_x, position_y, orientation, map, "
        "home_map, home_x, home_y) VALUES (%u, %u, '%s', %u, %u, %u, 1, "
        "1.0, 1.0, 0, 0, %.2f, %.2f, 0.0, %u, %u, %.2f, %.2f)",
        newGuid, session->getAccountId(), name.c_str(), classId, gender, portrait,
        startX, startY, startMapId, startMapId, startX, startY);

    if (!db.execute(query)) {
        LOG_ERROR("CharCreate: failed to insert player into database");
        GamePacket response(SMSG_CHAR_CREATE);
        response.writeUint8(0);
        response.writeString("Database error creating character.");
        session->send(response);
        return;
    }

    // Grant starting items for this class (CreateItem: classId, item, count)
    const auto& createItems = cache->getCreateItems(classId);
    uint32 slot = 0;
    for (const auto& ci : createItems) {
        char itemQuery[512];
        snprintf(itemQuery, sizeof(itemQuery),
            "INSERT INTO player_inventory (guid, slot, entry, count, durability, soulbound) "
            "VALUES (%u, %u, %u, %u, 100, 1)",
            newGuid, slot, ci.item, ci.count);
        db.execute(itemQuery);
        ++slot;
    }

    // Grant starting spells for this class (CreateSpell: classId, spell)
    const auto& createSpells = cache->getCreateSpells(classId);
    for (const auto& cs : createSpells) {
        char spellQuery[256];
        snprintf(spellQuery, sizeof(spellQuery),
            "INSERT INTO player_spells (guid, spell_id, level) VALUES (%u, %u, 1)",
            newGuid, cs.spell);
        db.execute(spellQuery);
    }

    LOG_INFO("CharCreate: created '%s' (guid=%u, class=%u) for account %u",
             name.c_str(), newGuid, classId, session->getAccountId());

    // Send success
    GamePacket response(SMSG_CHAR_CREATE);
    response.writeUint8(1); // success
    response.writeString(name);
    session->send(response);
}

// ============================================================================
//  Opcode 5 - Enter world
//  Large handler: loads full player from DB, creates ServerPlayer, sets all
//  fields, adds to map, sends initial state packets.
// ============================================================================
void PacketHandler::handleEnterWorld(Session* session, GamePacket& packet) {
    if (!requireAuth(session))
        return;

    uint32 charGuid = packet.readUint32();

    LOG_INFO("EnterWorld: account %u requesting guid %u", session->getAccountId(), charGuid);

    // Load full player record from DB
    PlayerDb playerDb;
    playerDb.setConnector(&DatabaseMgr::instance().getPlayerConnection());

    DbPlayerRecord record;
    if (!playerDb.loadPlayer(charGuid, record)) {
        LOG_ERROR("EnterWorld: failed to load player guid %u", charGuid);
        session->disconnect();
        return;
    }

    // Verify this character belongs to the authenticated account
    if (record.account != session->getAccountId()) {
        LOG_WARN("EnterWorld: account %u tried to load guid %u belonging to account %u",
                 session->getAccountId(), charGuid, record.account);
        session->disconnect();
        return;
    }

    // Create ServerPlayer and populate from DB record
    auto player = std::make_shared<ServerPlayer>();
    player->setGuid(record.guid);
    player->setName(record.name);
    player->setSession(session);
    player->setAccountId(record.account);
    player->setGM(record.isGM);
    player->setAdmin(record.isAdmin);
    player->setClassId(static_cast<uint8>(record.playerClass));
    player->setGender(static_cast<uint8>(record.gender));
    player->setPortrait(static_cast<uint8>(record.portrait));
    player->setLevel(static_cast<uint16>(record.level));
    player->setXp(record.xp);
    player->setMoney(record.money);
    player->setPosition(record.positionX, record.positionY);
    player->setOrientation(record.orientation);
    player->setMapId(record.map);
    player->setHome(record.homeMap, record.homeX, record.homeY);
    player->setPvpPoints(record.pvpPoints);
    player->setPkCount(record.pkCount);
    player->setPvpFlag(static_cast<uint8>(record.pvpFlag));
    player->setProgression(record.progression);
    player->setNumInvestedSpells(record.numInvestedSpells);
    player->setCombatRating(record.combatRating);
    player->setTimePlayed(record.timePlayed);

    // Set health/mana from percentages
    player->recalculateStats();
    player->setHealth(static_cast<int32>(record.hpPct * player->getMaxHealth()));
    player->setMana(static_cast<int32>(record.mpPct * player->getMaxMana()));

    // Load inventory items (DbItemData: slot, entry, affix, affixScore, gem1-4,
    //                       enchantLevel, soulbound, count, durability)
    for (const auto& dbItem : record.inventory) {
        ItemInstance item;
        item.entry        = dbItem.entry;
        item.affix        = dbItem.affix;
        item.affixScore   = dbItem.affixScore;
        item.gem1         = dbItem.gem1;
        item.gem2         = dbItem.gem2;
        item.gem3         = dbItem.gem3;
        item.gem4         = dbItem.gem4;
        item.enchantLevel = dbItem.enchantLevel;
        item.soulbound    = static_cast<uint8>(dbItem.soulbound);
        item.count        = dbItem.count;
        item.durability   = dbItem.durability;
        player->setInventoryItem(dbItem.slot, item);
    }

    // Load equipment (DbEquipmentData: entry, affix, affixScore, gem1-4,
    //                  enchantLevel, soulbound, durability)
    for (uint32 i = 0; i < record.equipment.size() && i < MAX_EQUIPMENT_SLOTS; ++i) {
        const auto& dbEquip = record.equipment[i];
        if (dbEquip.entry == 0)
            continue;
        // Equipment data is set separately from inventory
        // ServerPlayer stores equipment in its own array
    }

    // Load spells (DbSpellData: spellId, level)
    for (const auto& dbSpell : record.spells) {
        player->learnSpell(dbSpell.spellId, dbSpell.level);
    }

    // Load spell cooldowns (DbSpellCooldownData: spellId, startDate, endDate)
    for (const auto& cd : record.spellCooldowns) {
        // Parse date strings to int64 timestamps
        // For now, store with zero timestamps - the Spell system handles expiry
        player->addSpellCooldown(cd.spellId, 0, 0);
    }

    // Load quests (DbQuestData: questId, rewarded, objectiveCount1-4)
    for (const auto& dbQuest : record.quests) {
        if (dbQuest.rewarded) {
            player->acceptQuest(dbQuest.questId);
            player->completeQuest(dbQuest.questId);
        } else {
            player->acceptQuest(dbQuest.questId);
            player->updateQuestObjective(dbQuest.questId, 0, dbQuest.objectiveCount1);
            player->updateQuestObjective(dbQuest.questId, 1, dbQuest.objectiveCount2);
            player->updateQuestObjective(dbQuest.questId, 2, dbQuest.objectiveCount3);
            player->updateQuestObjective(dbQuest.questId, 3, dbQuest.objectiveCount4);
        }
    }

    // Load stat investments (DbStatInvestment: statType, amount)
    for (const auto& si : record.statInvestments) {
        player->investStat(si.statType, static_cast<int32>(si.amount));
    }

    // Load discovered waypoints
    for (Guid wp : record.waypoints) {
        player->discoverWaypoint(wp);
    }

    // Recalculate stats with equipment and investments applied
    player->recalculateStats();

    // Link session to player
    session->setPlayerGuid(charGuid);

    // Add player to the PlayerMgr
    PlayerMgr::instance().addPlayer(player);

    // Add player to map
    ServerMap* map = Server::instance().getMap(record.map);
    if (map) {
        player->setMap(map);
        map->addPlayer(player);
    } else {
        LOG_ERROR("EnterWorld: map %u not found, using default map", record.map);
        map = Server::instance().getMap(1);
        if (map) {
            player->setMapId(1);
            player->setMap(map);
            map->addPlayer(player);
        }
    }

    // Check guild membership
    Guild* guild = GuildMgr::instance().findGuild(player->getGuildInfo().guildId);
    if (guild) {
        guild->setMemberOnline(charGuid, true);
        guild->broadcastOnlineStatus(charGuid, true);
    }

    // --- Send initial state packets to client ---

    // SMSG_PLAYER - full player snapshot
    {
        GamePacket pkt(SMSG_PLAYER);
        pkt.writeUint32(player->getGuid());
        pkt.writeString(player->getName());
        pkt.writeUint8(player->getClassId());
        pkt.writeUint8(player->getGender());
        pkt.writeUint8(player->getPortrait());
        pkt.writeUint16(player->getLevel());
        pkt.writeInt32(player->getHealth());
        pkt.writeInt32(player->getMaxHealth());
        pkt.writeInt32(player->getMana());
        pkt.writeInt32(player->getMaxMana());
        pkt.writeUint32(player->getXp());
        pkt.writeUint32(player->getMoney());
        pkt.writeFloat(player->getPositionX());
        pkt.writeFloat(player->getPositionY());
        pkt.writeFloat(player->getOrientation());
        pkt.writeUint32(player->getMapId());
        // Stats
        for (int i = 0; i < static_cast<int>(StatType::Count); ++i) {
            pkt.writeInt32(player->getStat(static_cast<StatType>(i)));
        }
        pkt.writeInt32(player->getArmor());
        pkt.writeFloat(player->getMoveSpeed());
        pkt.writeInt32(player->getPvpPoints());
        pkt.writeInt32(player->getPkCount());
        pkt.writeUint8(player->getPvpFlag());
        pkt.writeUint32(player->getCombatRating());
        pkt.writeUint32(player->getProgression());
        pkt.writeUint8(player->isGM() ? 1 : 0);
        session->send(pkt);
    }

    // SMSG_INVENTORY
    {
        GamePacket pkt(SMSG_INVENTORY);
        for (int i = 0; i < MAX_INVENTORY_SLOTS; ++i) {
            const ItemInstance& item = player->getInventoryItem(i);
            pkt.writeUint32(item.entry);
            if (item.entry != 0) {
                pkt.writeUint32(item.count);
                pkt.writeUint32(item.affix);
                pkt.writeUint32(item.affixScore);
                pkt.writeUint32(item.gem1);
                pkt.writeUint32(item.gem2);
                pkt.writeUint32(item.gem3);
                pkt.writeUint32(item.gem4);
                pkt.writeUint32(item.enchantLevel);
                pkt.writeUint8(item.soulbound);
                pkt.writeUint32(item.durability);
            }
        }
        session->send(pkt);
    }

    // SMSG_SPELLBOOK
    {
        GamePacket pkt(SMSG_SPELLBOOK);
        const auto& spells = player->getSpells();
        pkt.writeUint16(static_cast<uint16>(spells.size()));
        for (const auto& spell : spells) {
            pkt.writeUint32(spell.spellId);
            pkt.writeUint32(spell.level);
        }
        // Cooldowns
        const auto& cooldowns = player->getCooldowns();
        pkt.writeUint16(static_cast<uint16>(cooldowns.size()));
        for (const auto& cd : cooldowns) {
            pkt.writeUint32(cd.spellId);
            pkt.writeInt32(static_cast<int32>(cd.endDate - Server::instance().getUnixTime()));
        }
        session->send(pkt);
    }

    // SMSG_QUEST_LIST
    {
        GamePacket pkt(SMSG_QUEST_LIST);
        const auto& quests = player->getQuests();
        pkt.writeUint16(static_cast<uint16>(quests.size()));
        for (const auto& q : quests) {
            pkt.writeUint32(q.questEntry);
            pkt.writeUint8(q.rewarded);
            for (int i = 0; i < MAX_QUEST_OBJECTIVES; ++i) {
                pkt.writeUint32(q.objectiveCount[i]);
            }
        }
        session->send(pkt);
    }

    // SMSG_BANK
    {
        GamePacket pkt(SMSG_BANK);
        for (int i = 0; i < MAX_BANK_SLOTS; ++i) {
            const ItemInstance& item = player->getBankItem(i);
            pkt.writeUint32(item.entry);
            if (item.entry != 0) {
                pkt.writeUint32(item.count);
                pkt.writeUint32(item.affix);
                pkt.writeUint32(item.affixScore);
                pkt.writeUint32(item.gem1);
                pkt.writeUint32(item.gem2);
                pkt.writeUint32(item.gem3);
                pkt.writeUint32(item.gem4);
                pkt.writeUint32(item.enchantLevel);
                pkt.writeUint8(item.soulbound);
                pkt.writeUint32(item.durability);
            }
        }
        session->send(pkt);
    }

    // SMSG_GUILD_ROSTER if in guild
    if (guild) {
        guild->sendRoster(player.get());
    }

    // Send existing players/NPCs/objects on the map to this player
    if (map) {
        // Other players on the map
        auto mapPlayers = map->getPlayers();
        for (const auto& other : mapPlayers) {
            if (other->getGuid() == charGuid)
                continue;

            GamePacket pkt(SMSG_PLAYER);
            pkt.writeUint32(other->getGuid());
            pkt.writeString(other->getName());
            pkt.writeUint8(other->getClassId());
            pkt.writeUint8(other->getGender());
            pkt.writeUint8(other->getPortrait());
            pkt.writeUint16(other->getLevel());
            pkt.writeInt32(other->getHealth());
            pkt.writeInt32(other->getMaxHealth());
            pkt.writeInt32(other->getMana());
            pkt.writeInt32(other->getMaxMana());
            pkt.writeFloat(other->getPositionX());
            pkt.writeFloat(other->getPositionY());
            pkt.writeFloat(other->getOrientation());
            session->send(pkt);
        }

        // Broadcast this player's entrance to other players on the map
        {
            GamePacket pkt(SMSG_PLAYER);
            pkt.writeUint32(player->getGuid());
            pkt.writeString(player->getName());
            pkt.writeUint8(player->getClassId());
            pkt.writeUint8(player->getGender());
            pkt.writeUint8(player->getPortrait());
            pkt.writeUint16(player->getLevel());
            pkt.writeInt32(player->getHealth());
            pkt.writeInt32(player->getMaxHealth());
            pkt.writeInt32(player->getMana());
            pkt.writeInt32(player->getMaxMana());
            pkt.writeFloat(player->getPositionX());
            pkt.writeFloat(player->getPositionY());
            pkt.writeFloat(player->getOrientation());
            map->broadcastPacketExcept(pkt, charGuid);
        }
    }

    LOG_INFO("EnterWorld: player '%s' (guid=%u) entered map %u",
             player->getName().c_str(), charGuid, player->getMapId());
}

// ============================================================================
//  Opcode 7 - Delete character
//  Reads uint32 guid, uint32 unknown.
// ============================================================================
void PacketHandler::handleDeleteChar(Session* session, GamePacket& packet) {
    if (!requireAuth(session))
        return;

    uint32 charGuid = packet.readUint32();
    uint32 unknown  = packet.readUint32();
    (void)unknown;

    LOG_INFO("DeleteChar: account %u requesting delete of guid %u", session->getAccountId(), charGuid);

    // Verify ownership - load the character and check account
    PlayerDb playerDb;
    playerDb.setConnector(&DatabaseMgr::instance().getPlayerConnection());

    DbPlayerRecord record;
    if (!playerDb.loadPlayer(charGuid, record) || record.account != session->getAccountId()) {
        LOG_WARN("DeleteChar: account %u does not own guid %u", session->getAccountId(), charGuid);
        return;
    }

    // Delete from database (cascade deletes inventory, spells, etc.)
    MySQLConnector& db = DatabaseMgr::instance().getPlayerConnection();
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM player WHERE guid = %u AND account = %u",
             charGuid, session->getAccountId());
    db.execute(query);

    // Delete related tables
    snprintf(query, sizeof(query), "DELETE FROM player_inventory WHERE guid = %u", charGuid);
    db.execute(query);
    snprintf(query, sizeof(query), "DELETE FROM player_equipment WHERE guid = %u", charGuid);
    db.execute(query);
    snprintf(query, sizeof(query), "DELETE FROM player_spells WHERE guid = %u", charGuid);
    db.execute(query);
    snprintf(query, sizeof(query), "DELETE FROM player_quests WHERE guid = %u", charGuid);
    db.execute(query);
    snprintf(query, sizeof(query), "DELETE FROM player_bank WHERE guid = %u", charGuid);
    db.execute(query);

    LOG_INFO("DeleteChar: deleted character guid %u for account %u", charGuid, session->getAccountId());

    // Refresh character list
    handleRequestCharList(session, packet);
}

// ============================================================================
//  Opcode 8 - Chat message
//  Reads channel (uint8), message (string), optionally target (string).
// ============================================================================
void PacketHandler::handleChatMessage(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 channel       = packet.readUint8();
    std::string message = packet.readString();

    std::string target;
    if (channel == static_cast<uint8>(ChatChannel::Whisper)) {
        target = packet.readString();
    }

    // Check for GM commands first
    if (ChatSystem::isCommand(message)) {
        ChatSystem::processCommand(player, message);
        return;
    }

    ChatSystem::handleChatMessage(player, channel, message, target);
}

// ============================================================================
//  Opcode 9/10/35 - Movement update
//  Reads x (float), y (float), orientation (float).
//  Updates player position and broadcasts to nearby players.
// ============================================================================
void PacketHandler::handleMovement(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    float x           = packet.readFloat();
    float y           = packet.readFloat();
    float orientation = packet.readFloat();

    player->setPosition(x, y);
    player->setOrientation(orientation);

    // Broadcast position to other players on the same map
    ServerMap* map = player->getMap();
    if (map) {
        GamePacket movePkt(SMSG_GAME_STATE_UPDATE_1);
        movePkt.writeUint32(player->getGuid());
        movePkt.writeFloat(x);
        movePkt.writeFloat(y);
        movePkt.writeFloat(orientation);
        map->broadcastPacketExcept(movePkt, player->getGuid());
    }
}

// ============================================================================
//  Opcode 11 - Target change
// ============================================================================
void PacketHandler::handleTargetChange(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 targetGuid = packet.readUint32();
    player->setTarget(targetGuid);
}

// ============================================================================
//  Opcode 12 - Inventory swap
//  Reads uint8 srcSlot, uint16 dstSlot.
// ============================================================================
void PacketHandler::handleInventorySwap(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8  srcSlot = packet.readUint8();
    uint16 dstSlot = packet.readUint16();

    if (srcSlot >= MAX_INVENTORY_SLOTS || dstSlot >= MAX_INVENTORY_SLOTS) {
        LOG_WARN("InventorySwap: invalid slot src=%u dst=%u", srcSlot, dstSlot);
        return;
    }

    // Perform the swap
    ItemInstance srcItem = player->getInventoryItem(srcSlot);
    ItemInstance dstItem = player->getInventoryItem(dstSlot);
    player->setInventoryItem(srcSlot, dstItem);
    player->setInventoryItem(dstSlot, srcItem);

    // Send updated inventory to client
    GamePacket response(SMSG_INVENTORY);
    for (int i = 0; i < MAX_INVENTORY_SLOTS; ++i) {
        const ItemInstance& item = player->getInventoryItem(i);
        response.writeUint32(item.entry);
        if (item.entry != 0) {
            response.writeUint32(item.count);
            response.writeUint32(item.affix);
            response.writeUint32(item.affixScore);
            response.writeUint32(item.gem1);
            response.writeUint32(item.gem2);
            response.writeUint32(item.gem3);
            response.writeUint32(item.gem4);
            response.writeUint32(item.enchantLevel);
            response.writeUint8(item.soulbound);
            response.writeUint32(item.durability);
        }
    }
    session->send(response);
}

// ============================================================================
//  Opcode 13 - Equipment swap
//  Reads 2 bytes (invSlot, equipSlot).
// ============================================================================
void PacketHandler::handleEquipmentSwap(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 invSlot   = packet.readUint8();
    uint8 equipSlot = packet.readUint8();

    if (invSlot >= MAX_INVENTORY_SLOTS || equipSlot >= MAX_EQUIPMENT_SLOTS) {
        LOG_WARN("EquipmentSwap: invalid slot inv=%u equip=%u", invSlot, equipSlot);
        return;
    }

    player->equipItem(invSlot);
    player->recalculateStats();
}

// ============================================================================
//  Opcode 14 - Use item
// ============================================================================
void PacketHandler::handleUseItem(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 slot = packet.readUint8();

    if (slot >= MAX_INVENTORY_SLOTS) {
        LOG_WARN("UseItem: invalid slot %u", slot);
        return;
    }

    const ItemInstance& item = player->getInventoryItem(slot);
    if (item.entry == 0) {
        LOG_WARN("UseItem: slot %u is empty", slot);
        return;
    }

    // Look up item template
    GameCache* cache = Server::instance().getGameCache();
    const ItemTemplate* tmpl = cache->getItemTemplate(item.entry);
    if (!tmpl) {
        LOG_WARN("UseItem: unknown item entry %u", item.entry);
        return;
    }

    // Execute the first spell attached to the item if present
    // ItemTemplate::spells[MAX_ITEM_SPELLS] holds spell IDs
    if (tmpl->spells[0] != 0) {
        const SpellTemplate* spellTmpl = cache->getSpellTemplate(tmpl->spells[0]);
        if (spellTmpl) {
            Spell spell(player, spellTmpl);
            spell.setTarget(player->getTargetGuid());
            SpellCastResult result = spell.validate();
            if (result == SpellCastResult::Success) {
                spell.cast();
                // Consume the item if stackCount > 0 (consumable)
                if (tmpl->stackCount > 1) {
                    player->removeInventoryItem(slot, 1);
                }
            }
        }
    }
}

// ============================================================================
//  Opcode 15 - Bank deposit/withdraw
// ============================================================================
void PacketHandler::handleBankDeposit(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 action  = packet.readUint8(); // 0 = deposit, 1 = withdraw
    uint8 srcSlot = packet.readUint8();
    uint8 dstSlot = packet.readUint8();

    if (action == 0) {
        // Deposit: inventory -> bank
        player->moveToBankSlot(srcSlot, dstSlot);
    } else {
        // Withdraw: bank -> inventory
        player->moveFromBankSlot(srcSlot, dstSlot);
    }
}

// ============================================================================
//  Opcode 16 - Bank swap
// ============================================================================
void PacketHandler::handleBankSwap(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 slot1 = packet.readUint8();
    uint8 slot2 = packet.readUint8();

    if (slot1 >= MAX_BANK_SLOTS || slot2 >= MAX_BANK_SLOTS) {
        LOG_WARN("BankSwap: invalid slot %u or %u", slot1, slot2);
        return;
    }

    LOG_DEBUG("BankSwap: swapping bank slots %u and %u", slot1, slot2);
    ItemInstance temp = player->getBankItem(slot1);
    player->setBankItem(slot1, player->getBankItem(slot2));
    player->setBankItem(slot2, temp);
}

// ============================================================================
//  Opcode 17 - Vendor buy
//  Reads npcGuid (uint32), itemIndex (uint32).
// ============================================================================
void PacketHandler::handleVendorBuy(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 npcGuid   = packet.readUint32();
    uint32 itemIndex = packet.readUint32();

    // Find the NPC on the player's map
    ServerMap* map = player->getMap();
    if (!map) return;

    ServerNpc* npc = map->findNpc(npcGuid);
    if (!npc) {
        LOG_WARN("VendorBuy: NPC guid %u not found", npcGuid);
        return;
    }

    // Check distance
    if (player->distanceTo(*npc) > 200.0f) {
        LOG_WARN("VendorBuy: NPC too far away");
        return;
    }

    // Get vendor items (VendorItem: npcEntry, item, maxCount, restockCooldown)
    GameCache* cache = Server::instance().getGameCache();
    const auto& vendorItems = cache->getVendorItems(npc->getEntry());
    if (itemIndex >= vendorItems.size()) {
        LOG_WARN("VendorBuy: invalid item index %u", itemIndex);
        return;
    }

    const VendorItem& vi = vendorItems[itemIndex];
    const ItemTemplate* itemTmpl = cache->getItemTemplate(vi.item);
    if (!itemTmpl) {
        LOG_WARN("VendorBuy: unknown item entry %u", vi.item);
        return;
    }

    // Check money (buy price is sell price * buyCostRatio)
    uint32 buyPrice = static_cast<uint32>(itemTmpl->sellPrice * itemTmpl->buyCostRatio);
    if (player->getMoney() < buyPrice) {
        ChatSystem::sendSystemMessage(player, "Not enough gold.");
        return;
    }

    // Check for free inventory slot
    int freeSlot = player->findFreeInventorySlot();
    if (freeSlot < 0) {
        ChatSystem::sendSystemMessage(player, "Inventory is full.");
        return;
    }

    // Deduct money and add item
    player->addMoney(-static_cast<int32>(buyPrice));
    ItemInstance newItem;
    newItem.entry      = vi.item;
    newItem.count      = 1;
    newItem.durability = itemTmpl->durability;
    player->addItemToInventory(newItem);

    // Notify client
    GamePacket notify(SMSG_NOTIFY_ITEM_ADD);
    notify.writeUint32(vi.item);
    notify.writeUint32(1); // count
    session->send(notify);
}

// ============================================================================
//  Opcode 18 - Guild MOTD
//  Reads string.
// ============================================================================
void PacketHandler::handleGuildMotd(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    std::string motd = packet.readString();

    if (!player->isInGuild()) {
        ChatSystem::sendSystemMessage(player, "You are not in a guild.");
        return;
    }

    GuildMgr::instance().setMotd(player->getGuildInfo().guildId, motd);
}

// ============================================================================
//  Opcode 20 - NPC interact / gossip
// ============================================================================
void PacketHandler::handleNpcInteract(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 npcGuid = packet.readUint32();

    ServerMap* map = player->getMap();
    if (!map) return;

    ServerNpc* npc = map->findNpc(npcGuid);
    if (!npc) {
        LOG_WARN("NpcInteract: NPC guid %u not found", npcGuid);
        return;
    }

    // Check distance
    if (player->distanceTo(*npc) > 200.0f) {
        LOG_WARN("NpcInteract: NPC too far away");
        return;
    }

    // Send gossip menu based on NPC template
    GameCache* cache = Server::instance().getGameCache();
    const NpcTemplate* npcTmpl = cache->getNpcTemplate(npc->getEntry());
    if (!npcTmpl) return;

    // Build gossip menu packet
    // GossipOption: entry, gossipId, text, requiredNpcFlag, clickNewGossip, clickScript
    GamePacket gossipPkt(SMSG_GOSSIP_MENU);
    gossipPkt.writeUint32(npcGuid);

    const auto& options = cache->getGossipOptions(npcTmpl->gossipMenuId);
    gossipPkt.writeUint8(static_cast<uint8>(options.size()));
    for (const auto& opt : options) {
        gossipPkt.writeUint32(opt.entry);
        gossipPkt.writeString(opt.text);
        gossipPkt.writeUint32(opt.requiredNpcFlag);
    }

    session->send(gossipPkt);
}

// ============================================================================
//  Opcode 22 - Quest accept/complete
// ============================================================================
void PacketHandler::handleQuestAction(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8  action  = packet.readUint8(); // 0 = accept, 1 = complete
    uint32 questId = packet.readUint32();
    uint32 npcGuid = packet.readUint32();
    (void)npcGuid;

    if (action == 0) {
        // Accept quest
        if (player->hasActiveQuest(questId) || player->hasCompletedQuest(questId)) {
            return;
        }
        player->acceptQuest(questId);

        // Send updated quest list
        GamePacket pkt(SMSG_QUEST_LIST);
        const auto& quests = player->getQuests();
        pkt.writeUint16(static_cast<uint16>(quests.size()));
        for (const auto& q : quests) {
            pkt.writeUint32(q.questEntry);
            pkt.writeUint8(q.rewarded);
            for (int i = 0; i < MAX_QUEST_OBJECTIVES; ++i) {
                pkt.writeUint32(q.objectiveCount[i]);
            }
        }
        session->send(pkt);
    } else {
        // Complete quest
        uint32 rewardChoice = 0;
        if (packet.buffer().remaining() >= 4) {
            rewardChoice = packet.readUint32();
        }
        player->completeQuest(questId, rewardChoice);
    }
}

// ============================================================================
//  Opcode 23 - Guild create
//  Reads string (guild name).
// ============================================================================
void PacketHandler::handleGuildCreate(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    std::string guildName = packet.readString();

    if (player->isInGuild()) {
        ChatSystem::sendSystemMessage(player, "You are already in a guild.");
        return;
    }

    if (guildName.empty() || guildName.length() > 24) {
        ChatSystem::sendSystemMessage(player, "Invalid guild name.");
        return;
    }

    // Check if name is already taken
    if (GuildMgr::instance().findGuildByName(guildName)) {
        ChatSystem::sendSystemMessage(player, "That guild name is already taken.");
        return;
    }

    Guild* guild = GuildMgr::instance().createGuild(guildName, player);
    if (guild) {
        LOG_INFO("Guild '%s' created by %s", guildName.c_str(), player->getName().c_str());
        guild->sendRoster(player);
    }
}

// ============================================================================
//  Opcode 24 - Guild invite
// ============================================================================
void PacketHandler::handleGuildInvite(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    std::string targetName = packet.readString();

    if (!player->isInGuild()) {
        ChatSystem::sendSystemMessage(player, "You are not in a guild.");
        return;
    }

    GuildMgr::instance().invitePlayer(player, targetName);
}

// ============================================================================
//  Opcode 25 - Party invite
// ============================================================================
void PacketHandler::handlePartyInvite(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    std::string targetName = packet.readString();

    PartyMgr::instance().invitePlayer(player, targetName);
}

// ============================================================================
//  Opcode 26 - Party accept/decline
// ============================================================================
void PacketHandler::handlePartyResponse(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 accepted = packet.readUint8();

    if (accepted) {
        PartyMgr::instance().acceptInvite(player);
    } else {
        PartyMgr::instance().declineInvite(player);
    }
}

// ============================================================================
//  Opcode 27 - Cast spell
//  Reads spellId (uint32), targetGuid (uint32), x (int32), y (int32),
//  flags (uint8).
// ============================================================================
void PacketHandler::handleCastSpell(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 spellId    = packet.readUint32();
    uint32 targetGuid = packet.readUint32();
    int32  x          = packet.readInt32();
    int32  y          = packet.readInt32();
    uint8  flags      = packet.readUint8();
    (void)flags;

    // Look up spell template
    GameCache* cache = Server::instance().getGameCache();
    const SpellTemplate* spellTmpl = cache->getSpellTemplate(spellId);
    if (!spellTmpl) {
        LOG_WARN("CastSpell: unknown spell %u", spellId);
        return;
    }

    // Check if player knows the spell
    if (!player->hasSpell(spellId)) {
        LOG_WARN("CastSpell: player %s does not know spell %u",
                 player->getName().c_str(), spellId);
        return;
    }

    // Create spell object and attempt to cast
    Spell spell(player, spellTmpl);
    spell.setTarget(targetGuid);
    spell.setTargetPos(static_cast<float>(x), static_cast<float>(y));

    const PlayerSpell* pSpell = player->getSpell(spellId);
    if (pSpell) {
        spell.setSpellLevel(static_cast<int32>(pSpell->level));
    }

    SpellCastResult result = spell.validate();
    if (result == SpellCastResult::Success) {
        spell.prepare();
        if (spellTmpl->castTime == 0) {
            // Instant cast
            spell.cast();
            spell.finish();
        }
        // Non-instant spells will be tracked by the player update loop
    } else {
        LOG_DEBUG("CastSpell: spell %u failed validation: %d", spellId, static_cast<int>(result));
    }
}

// ============================================================================
//  Opcode 28/43 - Auto attack toggle
// ============================================================================
void PacketHandler::handleAutoAttack(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 active = packet.readUint8();
    (void)active;

    // Auto-attack is handled by the combat system via the player's target.
    // When active=1, the player starts auto-attacking their current target.
    // When active=0, auto-attack stops.
    LOG_DEBUG("AutoAttack: player %s, active=%u, target=%u",
              player->getName().c_str(), active, player->getTargetGuid());
}

// ============================================================================
//  Opcode 29 - Set target
//  Reads uint32 targetGuid.
// ============================================================================
void PacketHandler::handleSetTarget(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 targetGuid = packet.readUint32();
    player->setTarget(targetGuid);
}

// ============================================================================
//  Opcode 30 - Loot action
// ============================================================================
void PacketHandler::handleLootAction(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 lootSourceGuid = packet.readUint32();
    uint8  lootSlot       = packet.readUint8();

    LOG_DEBUG("LootAction: player %s looting guid %u slot %u",
              player->getName().c_str(), lootSourceGuid, lootSlot);

    ServerMap* map = player->getMap();
    if (!map) return;

    // Try NPC corpse first
    ServerNpc* npc = map->findNpc(lootSourceGuid);
    if (npc) {
        if (player->distanceTo(*npc) > 200.0f) {
            LOG_WARN("LootAction: NPC too far away");
            return;
        }
        const auto& loot = npc->getLoot();
        if (lootSlot >= loot.size()) {
            LOG_WARN("LootAction: invalid loot slot %u (max %zu)", lootSlot, loot.size());
            return;
        }
        ItemInstance item = loot[lootSlot];
        if (item.isEmpty()) return;

        if (!player->addItemToInventory(item)) {
            ChatSystem::sendSystemMessage(player, "Inventory is full.");
            return;
        }

        // Notify client of item pickup
        GamePacket notify(SMSG_NOTIFY_ITEM_ADD);
        notify.writeUint32(item.entry);
        notify.writeUint32(item.count);
        session->send(notify);
        return;
    }

    // Try game object container
    ServerGameObj* obj = map->findGameObj(lootSourceGuid);
    if (obj) {
        if (player->distanceTo(*obj) > 200.0f) {
            LOG_WARN("LootAction: object too far away");
            return;
        }
        const auto& loot = obj->getLoot();
        if (lootSlot >= loot.size()) return;
        ItemInstance item = loot[lootSlot];
        if (item.isEmpty()) return;

        if (!player->addItemToInventory(item)) {
            ChatSystem::sendSystemMessage(player, "Inventory is full.");
            return;
        }

        GamePacket notify(SMSG_NOTIFY_ITEM_ADD);
        notify.writeUint32(item.entry);
        notify.writeUint32(item.count);
        session->send(notify);
    }
}

// ============================================================================
//  Opcode 31 - Trade request
// ============================================================================
void PacketHandler::handleTradeRequest(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 targetGuid = packet.readUint32();

    LOG_DEBUG("TradeRequest: player %s wants to trade with %u",
              player->getName().c_str(), targetGuid);

    // Find target player
    ServerPlayer* target = PlayerMgr::instance().findPlayer(targetGuid);
    if (!target) {
        ChatSystem::sendSystemMessage(player, "Player not found.");
        return;
    }

    // Check same map
    if (player->getMap() != target->getMap()) {
        ChatSystem::sendSystemMessage(player, "Player is not nearby.");
        return;
    }

    // Check distance
    if (player->distanceTo(*target) > 200.0f) {
        ChatSystem::sendSystemMessage(player, "Player is too far away.");
        return;
    }

    // Send trade offer to target
    GamePacket tradePkt(SMSG_TRADE_UPDATE);
    tradePkt.writeUint32(player->getGuid());
    tradePkt.writeString(player->getName());
    target->sendPacket(tradePkt);
}

// ============================================================================
//  Opcode 52/53 - Game object interact
// ============================================================================
void PacketHandler::handleGameObjInteract(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 objGuid = packet.readUint32();

    ServerMap* map = player->getMap();
    if (!map) return;

    ServerGameObj* obj = map->findGameObj(objGuid);
    if (!obj) {
        LOG_WARN("GameObjInteract: object guid %u not found", objGuid);
        return;
    }

    // Check distance
    if (player->distanceTo(*obj) > 200.0f) {
        LOG_WARN("GameObjInteract: object too far away");
        return;
    }

    LOG_DEBUG("GameObjInteract: player %s interacting with object %u (entry %u)",
              player->getName().c_str(), objGuid, obj->getEntry());

    obj->interact(player);
}

// ============================================================================
//  Opcode 53 - Spell cancel
// ============================================================================
void PacketHandler::handleSpellCancel(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    LOG_DEBUG("SpellCancel: player %s", player->getName().c_str());

    if (player->isCasting()) {
        player->getCurrentSpell()->cancel();
        player->clearCurrentSpell();
    }
}

// ============================================================================
//  Opcode 54 - Gossip option select
// ============================================================================
void PacketHandler::handleGossipSelect(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 npcGuid  = packet.readUint32();
    uint32 optionId = packet.readUint32();

    LOG_DEBUG("GossipSelect: player %s selected option %u on NPC %u",
              player->getName().c_str(), optionId, npcGuid);

    ServerMap* map = player->getMap();
    if (!map) return;

    ServerNpc* npc = map->findNpc(npcGuid);
    if (!npc) return;

    // Validate distance
    if (player->distanceTo(*npc) > 200.0f) return;

    // Look up the gossip option and execute its script
    GameCache* cache = Server::instance().getGameCache();
    const NpcTemplate* npcTmpl = cache->getNpcTemplate(npc->getEntry());
    if (!npcTmpl) return;

    // GossipOption: entry, gossipId, text, requiredNpcFlag, clickNewGossip, clickScript
    const auto& options = cache->getGossipOptions(npcTmpl->gossipMenuId);
    for (const auto& opt : options) {
        if (opt.entry == optionId) {
            // If this option opens a new gossip page
            if (opt.clickNewGossip != 0) {
                GamePacket gossipPkt(SMSG_GOSSIP_MENU);
                gossipPkt.writeUint32(npcGuid);
                const auto& newOptions = cache->getGossipOptions(opt.clickNewGossip);
                gossipPkt.writeUint8(static_cast<uint8>(newOptions.size()));
                for (const auto& newOpt : newOptions) {
                    gossipPkt.writeUint32(newOpt.entry);
                    gossipPkt.writeString(newOpt.text);
                    gossipPkt.writeUint32(newOpt.requiredNpcFlag);
                }
                session->send(gossipPkt);
            }

            // Execute script if present
            if (opt.clickScript != 0) {
                const auto& scripts = cache->getScriptEntries(opt.clickScript);
                for (const auto& script : scripts) {
                    LOG_DEBUG("GossipSelect: executing script command %u for entry %u",
                              script.command, script.entry);
                    // Script execution is handled by the scripting system
                }
            }
            break;
        }
    }
}

// ============================================================================
//  Opcode 55/56/67 - Arena queue
// ============================================================================
void PacketHandler::handleArenaQueue(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 action = packet.readUint8(); // 0=queue solo, 1=queue party, 2=dequeue

    switch (action) {
        case 0:
            ArenaMgr::instance().queueSolo(player);
            break;
        case 1:
            ArenaMgr::instance().queueParty(player);
            break;
        case 2:
            ArenaMgr::instance().dequeue(player);
            break;
        default:
            LOG_WARN("ArenaQueue: unknown action %u", action);
            break;
    }
}

// ============================================================================
//  Opcode 47 - Stat invest
// ============================================================================
void PacketHandler::handleStatInvest(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8 statType = packet.readUint8();
    uint8 amount   = packet.readUint8();

    if (statType >= static_cast<uint8>(StatType::Count)) {
        LOG_WARN("StatInvest: invalid stat type %u", statType);
        return;
    }

    player->investStat(statType, static_cast<int32>(amount));
    player->recalculateStats();
}

// ============================================================================
//  Opcode 59 - Guild invite response
// ============================================================================
void PacketHandler::handleGuildInviteResponse(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8  accepted = packet.readUint8();
    uint32 guildId  = packet.readUint32();

    if (accepted) {
        GuildMgr::instance().acceptInvite(player, guildId);
    } else {
        GuildMgr::instance().declineInvite(player, guildId);
    }
}

// ============================================================================
//  Opcode 60 - Guild leave
// ============================================================================
void PacketHandler::handleGuildLeave(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    if (!player->isInGuild()) {
        return;
    }

    GuildMgr::instance().removeMember(player->getGuildInfo().guildId, player->getGuid());
    player->getGuildInfo().guildId = 0;
    player->getGuildInfo().rank    = 0;

    ChatSystem::sendSystemMessage(player, "You have left the guild.");
}

// ============================================================================
//  Opcode 61 - Guild kick
// ============================================================================
void PacketHandler::handleGuildKick(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 targetGuid = packet.readUint32();

    if (!player->isInGuild()) {
        return;
    }

    // Only guild leader can kick
    Guild* guild = GuildMgr::instance().findGuild(player->getGuildInfo().guildId);
    if (!guild || guild->getLeaderGuid() != player->getGuid()) {
        ChatSystem::sendSystemMessage(player, "Only the guild leader can kick members.");
        return;
    }

    GuildMgr::instance().removeMember(player->getGuildInfo().guildId, targetGuid);

    // Update the kicked player if online
    ServerPlayer* target = PlayerMgr::instance().findPlayer(targetGuid);
    if (target) {
        target->getGuildInfo().guildId = 0;
        target->getGuildInfo().rank    = 0;
        ChatSystem::sendSystemMessage(target, "You have been kicked from the guild.");
    }
}

// ============================================================================
//  Opcode 62 - Guild rank change
// ============================================================================
void PacketHandler::handleGuildRankChange(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 targetGuid = packet.readUint32();
    uint32 newRank    = packet.readUint32();

    if (!player->isInGuild()) {
        return;
    }

    Guild* guild = GuildMgr::instance().findGuild(player->getGuildInfo().guildId);
    if (!guild || guild->getLeaderGuid() != player->getGuid()) {
        ChatSystem::sendSystemMessage(player, "Only the guild leader can change ranks.");
        return;
    }

    GuildMgr::instance().setMemberRank(player->getGuildInfo().guildId, targetGuid, newRank);
}

// ============================================================================
//  Opcode 63 - Party changes
// ============================================================================
void PacketHandler::handlePartyChanges(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8  action     = packet.readUint8();
    uint32 targetGuid = packet.readUint32();

    LOG_DEBUG("PartyChanges: player %s action=%u target=%u",
              player->getName().c_str(), action, targetGuid);

    ServerParty* party = PartyMgr::instance().findPartyByMember(player->getGuid());
    if (!party) return;

    switch (action) {
        case 0: // Kick member
            if (party->getLeaderGuid() != player->getGuid()) {
                ChatSystem::sendSystemMessage(player, "Only the party leader can kick members.");
                return;
            }
            party->removeMember(targetGuid);
            if (party->getMemberCount() <= 1) {
                PartyMgr::instance().disbandParty(party);
            } else {
                party->broadcastPartyList();
            }
            break;
        case 1: // Promote leader
            if (party->getLeaderGuid() != player->getGuid()) {
                ChatSystem::sendSystemMessage(player, "Only the party leader can promote.");
                return;
            }
            if (!party->hasMember(targetGuid)) return;
            party->setLeader(targetGuid);
            party->broadcastPartyList();
            break;
        default:
            LOG_WARN("PartyChanges: unknown action %u", action);
            break;
    }
}

// ============================================================================
//  Opcode 64 - Party leave
// ============================================================================
void PacketHandler::handlePartyLeave(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    ServerParty* party = PartyMgr::instance().findPartyByMember(player->getGuid());
    if (!party) {
        return;
    }

    party->removeMember(player->getGuid());

    // If party is empty or has 1 member, disband
    if (party->getMemberCount() <= 1) {
        PartyMgr::instance().disbandParty(party);
    } else {
        party->broadcastPartyList();
    }
}

// ============================================================================
//  Opcode 65 - Mail send
// ============================================================================
void PacketHandler::handleMailSend(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    std::string recipient = packet.readString();
    uint8  hasAttachment  = packet.readUint8();
    uint32 attachSlot     = 0;
    if (hasAttachment) {
        attachSlot = packet.readUint32();
    }

    LOG_DEBUG("MailSend: from %s to %s, attachment=%u slot=%u",
              player->getName().c_str(), recipient.c_str(), hasAttachment, attachSlot);

    // Find recipient by name (online first)
    ServerPlayer* target = PlayerMgr::instance().findPlayerByName(recipient);

    ItemInstance attachment;
    if (hasAttachment) {
        if (attachSlot >= MAX_INVENTORY_SLOTS) return;
        attachment = player->getInventoryItem(attachSlot);
        if (attachment.isEmpty()) {
            ChatSystem::sendSystemMessage(player, "No item in that slot.");
            return;
        }
        // Remove from sender inventory
        player->removeInventoryItem(attachSlot);
    }

    // If recipient is online, add to their mail directly
    if (target) {
        if (hasAttachment) {
            target->addMail(attachment);
        }
        ChatSystem::sendSystemMessage(target,
            std::string("You have received mail from ") + player->getName() + ".");
    } else {
        // Insert into DB for offline delivery
        if (hasAttachment) {
            auto& db = DatabaseMgr::instance().getPlayerConnection();
            char buf[1024];
            // Look up recipient guid from DB
            ResultSet rs;
            snprintf(buf, sizeof(buf),
                "SELECT `guid` FROM `player` WHERE `name` = '%s'",
                db.escapeString(recipient).c_str());
            if (db.query(buf, rs) && !rs.empty()) {
                uint32 recipGuid = static_cast<uint32>(std::stoul(rs[0]["guid"]));
                snprintf(buf, sizeof(buf),
                    "INSERT INTO `player_mail` (`guid`, `sender_guid`, `entry`, `affix`, "
                    "`affix_score`, `gem1`, `gem2`, `gem3`, `gem4`, `enchant_level`, "
                    "`count`, `soulbound`, `durability`, `sent_date`) "
                    "VALUES('%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', '%u', "
                    "'%u', '%u', '%d', '%lld')",
                    recipGuid, player->getGuid(),
                    attachment.entry, attachment.affix, attachment.affixScore,
                    attachment.gem1, attachment.gem2, attachment.gem3, attachment.gem4,
                    attachment.enchantLevel, attachment.count,
                    attachment.soulbound ? 1 : 0, attachment.durability,
                    (long long)std::time(nullptr));
                db.execute(buf);
            } else {
                ChatSystem::sendSystemMessage(player, "Player not found.");
                // Return the item
                player->addItemToInventory(attachment);
                return;
            }
        }
    }

    ChatSystem::sendSystemMessage(player, "Mail sent.");
}

// ============================================================================
//  Opcode 66 - Mail action (read, delete, take attachment)
// ============================================================================
void PacketHandler::handleMailAction(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint8  action = packet.readUint8();
    uint32 mailId = packet.readUint32();

    LOG_DEBUG("MailAction: player %s action=%u mailId=%u",
              player->getName().c_str(), action, mailId);

    auto& db = DatabaseMgr::instance().getPlayerConnection();
    char buf[512];

    switch (action) {
        case 0: // Read -- no-op server side, client handles display
            break;
        case 1: { // Take attachment
            ResultSet rs;
            snprintf(buf, sizeof(buf),
                "SELECT `entry`, `affix`, `affix_score`, `gem1`, `gem2`, `gem3`, `gem4`, "
                "`enchant_level`, `count`, `soulbound`, `durability` "
                "FROM `player_mail` WHERE `id` = '%u' AND `guid` = '%u'",
                mailId, player->getGuid());
            if (db.query(buf, rs) && !rs.empty()) {
                ItemInstance item;
                item.entry = static_cast<uint32>(std::stoul(rs[0]["entry"]));
                item.count = static_cast<uint32>(std::stoul(rs[0]["count"]));
                if (item.entry != 0) {
                    if (!player->addItemToInventory(item)) {
                        ChatSystem::sendSystemMessage(player, "Inventory is full.");
                        return;
                    }
                    GamePacket notify(SMSG_NOTIFY_ITEM_ADD);
                    notify.writeUint32(item.entry);
                    notify.writeUint32(item.count);
                    session->send(notify);
                }
            }
            // Delete mail after taking attachment
            snprintf(buf, sizeof(buf),
                "DELETE FROM `player_mail` WHERE `id` = '%u' AND `guid` = '%u'",
                mailId, player->getGuid());
            db.execute(buf);
            break;
        }
        case 2: // Delete
            snprintf(buf, sizeof(buf),
                "DELETE FROM `player_mail` WHERE `id` = '%u' AND `guid` = '%u'",
                mailId, player->getGuid());
            db.execute(buf);
            break;
        default:
            LOG_WARN("MailAction: unknown action %u", action);
            break;
    }
}

// ============================================================================
//  Opcode 58 - Waypoint teleport
// ============================================================================
void PacketHandler::handleWaypointTeleport(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 waypointGuid = packet.readUint32();

    // Check if player has discovered this waypoint
    if (!player->hasDiscoveredWaypoint(waypointGuid)) {
        ChatSystem::sendSystemMessage(player, "You haven't discovered that waypoint.");
        return;
    }

    // Find the game object for this waypoint on the current map
    ServerMap* map = player->getMap();
    if (!map) return;

    ServerGameObj* waypointObj = map->findGameObj(waypointGuid);

    // Cross-map teleportation: search all maps if not found on current map
    ServerMap* targetMap = map;
    if (!waypointObj) {
        // Search all maps for this waypoint
        for (auto& [mapId, serverMap] : Server::instance().getMaps()) {
            waypointObj = serverMap->findGameObj(waypointGuid);
            if (waypointObj) {
                targetMap = serverMap.get();
                break;
            }
        }
        if (!waypointObj) {
            LOG_WARN("WaypointTeleport: waypoint %u not found on any map", waypointGuid);
            return;
        }
    }

    // If cross-map, move player between maps
    if (targetMap != map) {
        auto playerShared = std::shared_ptr<ServerPlayer>(
            PlayerMgr::instance().findPlayer(player->getGuid()),
            [](ServerPlayer*) {}  // no-delete, PlayerMgr owns
        );
        // Find real shared_ptr from the map's player list
        auto players = map->getPlayers();
        for (auto& p : players) {
            if (p->getGuid() == player->getGuid()) {
                playerShared = p;
                break;
            }
        }
        map->removePlayer(player->getGuid());
        player->setMapId(targetMap->getId());
        targetMap->addPlayer(playerShared);
    }

    // Teleport player to waypoint position
    player->setPosition(waypointObj->getPositionX(), waypointObj->getPositionY());

    // Send movement update to player and broadcast to others
    GamePacket movePkt(SMSG_GAME_STATE_UPDATE_1);
    movePkt.writeUint32(player->getGuid());
    movePkt.writeFloat(player->getPositionX());
    movePkt.writeFloat(player->getPositionY());
    movePkt.writeFloat(player->getOrientation());
    session->send(movePkt);
    targetMap->broadcastPacketExcept(movePkt, player->getGuid());
}

// ============================================================================
//  Opcode 70 - Inspect request
// ============================================================================
void PacketHandler::handleInspect(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 targetGuid = packet.readUint32();

    ServerPlayer* target = PlayerMgr::instance().findPlayer(targetGuid);
    if (!target) {
        LOG_WARN("Inspect: target guid %u not found", targetGuid);
        return;
    }

    // Build SMSG_INSPECT with target's equipment and stats
    GamePacket response(SMSG_INSPECT);
    response.writeUint32(target->getGuid());
    response.writeString(target->getName());
    response.writeUint8(target->getClassId());
    response.writeUint16(target->getLevel());

    // Equipment
    for (int i = 0; i < MAX_EQUIPMENT_SLOTS; ++i) {
        const ItemInstance& equip = target->getEquipment(i);
        response.writeUint32(equip.entry);
        if (equip.entry != 0) {
            response.writeUint32(equip.affix);
            response.writeUint32(equip.affixScore);
            response.writeUint32(equip.gem1);
            response.writeUint32(equip.gem2);
            response.writeUint32(equip.gem3);
            response.writeUint32(equip.gem4);
            response.writeUint32(equip.enchantLevel);
        }
    }

    // Stats
    for (int i = 0; i < static_cast<int>(StatType::Count); ++i) {
        response.writeInt32(target->getStat(static_cast<StatType>(i)));
    }
    response.writeInt32(target->getArmor());

    session->send(response);
}

// ============================================================================
//  Opcode 45 - Combine items (crafting)
//  CombineRecipe: itemId1, itemId2, outputItem
// ============================================================================
void PacketHandler::handleCombineItems(Session* session, GamePacket& packet) {
    if (!requireInWorld(session))
        return;

    ServerPlayer* player = getPlayer(session);
    if (!player) return;

    uint32 recipeIndex = packet.readUint32();

    GameCache* cache = Server::instance().getGameCache();
    const auto& recipes = cache->getCombineRecipes();

    if (recipeIndex >= recipes.size()) {
        LOG_WARN("CombineItems: invalid recipe index %u", recipeIndex);
        return;
    }

    const CombineRecipe& recipe = recipes[recipeIndex];

    // Check if player has both required items
    if (recipe.itemId1 != 0 && !player->hasItem(recipe.itemId1, 1)) {
        ChatSystem::sendSystemMessage(player, "Missing required materials.");
        return;
    }
    if (recipe.itemId2 != 0 && !player->hasItem(recipe.itemId2, 1)) {
        ChatSystem::sendSystemMessage(player, "Missing required materials.");
        return;
    }

    // Consume materials
    if (recipe.itemId1 != 0) {
        for (int slot = 0; slot < MAX_INVENTORY_SLOTS; ++slot) {
            const ItemInstance& item = player->getInventoryItem(slot);
            if (item.entry == recipe.itemId1) {
                player->removeInventoryItem(slot, 1);
                break;
            }
        }
    }
    if (recipe.itemId2 != 0) {
        for (int slot = 0; slot < MAX_INVENTORY_SLOTS; ++slot) {
            const ItemInstance& item = player->getInventoryItem(slot);
            if (item.entry == recipe.itemId2) {
                player->removeInventoryItem(slot, 1);
                break;
            }
        }
    }

    // Create result item
    ItemInstance result;
    result.entry = recipe.outputItem;
    result.count = 1;

    const ItemTemplate* resultTmpl = cache->getItemTemplate(result.entry);
    if (resultTmpl) {
        result.durability = resultTmpl->durability;
    }

    player->addItemToInventory(result);

    // Notify
    GamePacket notify(SMSG_NOTIFY_ITEM_ADD);
    notify.writeUint32(result.entry);
    notify.writeUint32(result.count);
    session->send(notify);

    LOG_DEBUG("CombineItems: player %s crafted item %u",
              player->getName().c_str(), result.entry);
}
