#include "Application.h"
#include "Log.h"
#include "Config.h"
#include "DmystVersion.h"
#include "network/Connection.h"
#include "network/PacketHandler.h"
#include "network/PacketSender.h"
#include "resource/ResourceManager.h"
#include "resource/ContentDB.h"
#include "render/SpriteManager.h"
#include "resource/MapLoader.h"
#include "game/WorldState.h"
#include "ui/FontManager.h"
#include "ui/UIManager.h"
#include "ui/hud/UnitFrame.h"
#include "ui/hud/Toolbar.h"
#include "ui/hud/ChatWindow.h"

#include <SFML/Graphics.hpp>
#include <ctime>
#include <filesystem>

// Application implementation decompiled from Dreadmyst.exe r1189.
//
// Key binary functions:
//   0x00403ea0 - getInstance (singleton)
//   0x00405110 - constructor
//   0x004074a0 - initConfig
//   0x00407f10 - createSfmlWindow
//   0x00405650 - run (main game loop)
//   0x00405d40 - input
//   0x004061a0 - render
//   0x00406a80 - showLoadingScreen
//   0x004088f0 - toggleFullscreen
//   0x00407d20 - onResize

Application& Application::instance()
{
    static Application inst;
    return inst;
}

Application::Application()
{
    initConfig();
    initResources();
}

// Binary: FUN_004074a0
// Reads config.ini from working directory (binary uses %APPDATA%\Dreadmyst\config.ini on Windows).
// On Linux we just read from the current directory or alongside the binary.
void Application::initConfig()
{
    // Try local config first, then fallback paths
    if (!sConfig->load("config.ini"))
    {
        // Try the assets directory
        if (!sConfig->load("../dreadmyst-scripts-db-assets/config.ini"))
        {
            LOG_WARN("Could not load config.ini, using defaults");
        }
    }

    m_devMode = sConfig->getBool("System", "DevMode", false);

    LOG_INFO("Config loaded: DevMode=%s", m_devMode ? "true" : "false");
}

// Phase 3: Initialize resource systems (ContentDB, ResourceManager).
// Binary: ContentMgr initialization scans ZIP files in content/ directory.
void Application::initResources()
{
    namespace fs = std::filesystem;

    // Find asset base path: try local ../dreadmyst-scripts-db-assets/ first
    std::vector<std::string> candidates = {
        "../dreadmyst-scripts-db-assets",
        "../../dreadmyst-scripts-db-assets",
        "../../../dreadmyst-scripts-db-assets",
    };

    for (const auto& p : candidates)
    {
        if (fs::exists(p) && fs::is_directory(p))
        {
            m_assetPath = p;
            break;
        }
    }

    if (m_assetPath.empty())
    {
        LOG_WARN("Asset directory not found, map loading will fail");
        return;
    }

    LOG_INFO("Asset path: %s", m_assetPath.c_str());

    // Open game database
    std::string dbPath = m_assetPath + "/game.db";
    if (!sContentDB.open(dbPath))
    {
        LOG_WARN("Could not open game.db at '%s'", dbPath.c_str());
    }

    // Scan content ZIPs for texture assets
    std::string contentPath = m_assetPath + "/content";
    sResourceMgr.scanDirectory(contentPath);

    // Init SpriteManager with asset base path for model scripts
    sSpriteMgr.init(m_assetPath);

    // Phase 5: Load fonts
    std::string fontPath = m_assetPath + "/content/fonts";
    if (!sFontMgr.init(fontPath))
    {
        // Try alternate location
        fontPath = "assets/content/fonts";
        sFontMgr.init(fontPath);
    }
}

// Binary: FUN_00407f10
// Creates the SFML window with settings from config.ini.
// Reads: Window.Width (default 1280, min 1280), Window.Height (default 720, min 720),
//        Window.MaxFPS (default 300), Window.Fullscreen, Window.Resize,
//        Window.OriginalDPI, Window.WindowName (default "Application").
void Application::createWindow()
{
    // Read config values
    m_originalDPI = sConfig->getBool("Window", "OriginalDPI", false);
    m_fullscreen  = sConfig->getBool("Window", "Fullscreen", false);
    m_useCanvas   = !m_originalDPI;

    m_windowName = sConfig->getString("Window", "WindowName", "Dreadmyst");
    std::string title = m_windowName + " " + std::to_string(DMYST_REVISION);

    if (!m_fullscreen)
    {
        // Binary: default width=0x500(1280), min 1280; default height=0x2D0(720), min 720
        m_width = static_cast<unsigned int>(sConfig->getInt("Window", "Width", 1280));
        if (m_width < 1280) m_width = 1280;

        m_height = static_cast<unsigned int>(sConfig->getInt("Window", "Height", 720));
        if (m_height < 720) m_height = 720;
    }
    else
    {
        // Fullscreen: use desktop resolution
        auto desktop = sf::VideoMode::getDesktopMode();
        m_width  = desktop.size.x;
        m_height = desktop.size.y;
    }

    // MaxFPS: binary default 300
    m_maxFps = static_cast<unsigned int>(sConfig->getInt("Window", "MaxFPS", 300));

    // Create the window
    bool resizable = sConfig->getBool("Window", "Resize", true);

    // SFML 3 style flags
    auto style = sf::Style::Close | sf::Style::Titlebar;
    if (resizable)
        style |= sf::Style::Resize;

    if (m_fullscreen)
    {
        // Binary uses borderless fullscreen (removes WS_CAPTION, WS_THICKFRAME)
        // In SFML 3 on Linux, we just create a borderless window at desktop resolution
        style = sf::Style::None;
    }

    auto settings = sf::ContextSettings{};
    settings.depthBits = 8;

    m_window.create(sf::VideoMode({m_width, m_height}, 32), title, style, sf::State::Windowed, settings);

    if (!m_window.isOpen())
    {
        LOG_ERROR("Failed to open SFML window.");
        return;
    }

    m_window.setFramerateLimit(m_maxFps);
    m_window.setVerticalSyncEnabled(false);
    m_window.setKeyRepeatEnabled(false);

    // Create the off-screen render texture for scaled rendering
    if (m_useCanvas)
    {
        if (!m_canvas.resize({m_width, m_height}))
        {
            LOG_ERROR("Failed to create RenderTexture %ux%u", m_width, m_height);
            m_useCanvas = false;
        }
    }

    // Load window icon: "content/icon.png"
    sf::Image icon;
    if (icon.loadFromFile("content/icon.png"))
    {
        m_window.setIcon(icon.getSize(), icon.getPixelsPtr());
    }

    LOG_INFO("Window created: %ux%u, MaxFPS=%u, Fullscreen=%s, OriginalDPI=%s",
             m_width, m_height, m_maxFps,
             m_fullscreen ? "true" : "false",
             m_originalDPI ? "true" : "false");

    showLoadingScreen();
}

// Binary: FUN_00406a80
// Loads "content/loading.png", centers on screen, draws once.
void Application::showLoadingScreen()
{
    sf::Texture loadingTex;
    if (!loadingTex.loadFromFile("content/loading.png"))
        return;

    sf::Sprite loadingSprite(loadingTex);

    // Center the loading image
    auto texSize = loadingTex.getSize();
    float x = (static_cast<float>(m_width) - static_cast<float>(texSize.x)) / 2.0f;
    float y = (static_cast<float>(m_height) - static_cast<float>(texSize.y)) / 2.0f;
    loadingSprite.setPosition({x, y});

    // Clear and draw
    sf::RenderTarget& target = getActiveTarget();
    target.clear(sf::Color::Black);
    target.draw(loadingSprite);

    if (m_useCanvas)
    {
        m_canvas.display();
        // Blit canvas to window
        sf::Sprite canvasSprite(m_canvas.getTexture());
        m_window.clear(sf::Color::Black);
        m_window.draw(canvasSprite);
    }

    m_window.display();
}

sf::RenderTarget& Application::getActiveTarget()
{
    if (m_useCanvas)
        return m_canvas;
    return m_window;
}

// Binary: FUN_00405650
// Main game loop. Sequence per frame:
//   1. Clock restart -> deltaTime
//   2. Mouse position
//   3. ContentManager update (with >10ms perf warning)
//   4. Input processing (with >10ms perf warning)
//   5. Network update (binary: inside render object updates at FUN_004592d0)
//   6. Clear canvas
//   7. Render (with >10ms perf warning)
//   8. Display
//   9. Handle close/fullscreen/reconnect
void Application::run()
{
    createWindow();

    if (!m_window.isOpen())
    {
        LOG_ERROR("Failed to open SFML window.");
        return;
    }

    // Phase 1: Connect to game server and send auth.
    // Binary: login flow at FUN_0049d5e0 does HTTP POST for token, then TCP connect.
    // Simplified: direct TCP connect + auth with test token.
    if (sConnection.connect())
    {
        // Try login with username/password from config.ini [Auth] section.
        // Falls back to token auth if no credentials configured.
        std::string username = sConfig->getString("Auth", "Username", "");
        std::string password = sConfig->getString("Auth", "Password", "");
        if (!username.empty() && !password.empty())
        {
            PacketSender::sendLogin(username, password, DMYST_REVISION);
        }
        else
        {
            // Fallback: token-based auth (legacy)
            std::string token = sConfig->getString("Auth", "Token", "test");
            PacketSender::sendAuthenticate(token, DMYST_REVISION);
        }
    }

    LOG_INFO("Entering main game loop");

    while (m_window.isOpen())
    {
        // Delta time
        m_deltaTime = m_clock.restart();

        // Mouse position
        m_mousePos = sf::Mouse::getPosition(m_window);

        // Input
        clock_t t0 = clock();
        input();
        clock_t t1 = clock();
        int inputMs = static_cast<int>((t1 - t0) * 1000 / CLOCKS_PER_SEC);
        if (inputMs > 10)
            LOG_WARN("input() took %dms", inputMs);

        // Network update
        // Binary: FUN_004592d0 calls SfSocket::update() then processes all queued packets.
        // Called inside render object update chain; we call it directly in the main loop.
        updateNetwork();

        // Game state progression (auto-flow for Phase 2)
        updateGameState();

        // Clear
        sf::RenderTarget& target = getActiveTarget();
        target.clear(sf::Color::Black);

        // Render
        t0 = clock();
        render();
        t1 = clock();
        int renderMs = static_cast<int>((t1 - t0) * 1000 / CLOCKS_PER_SEC);
        if (renderMs > 10)
            LOG_WARN("render() took %dms", renderMs);

        // If using canvas (non-OriginalDPI), blit RenderTexture to window
        if (m_useCanvas)
        {
            m_canvas.display();
            sf::Sprite canvasSprite(m_canvas.getTexture());

            // Scale canvas to fill window
            auto winSize = m_window.getSize();
            canvasSprite.setScale({
                static_cast<float>(winSize.x) / static_cast<float>(m_width),
                static_cast<float>(winSize.y) / static_cast<float>(m_height)
            });

            m_window.clear(sf::Color::Black);
            m_window.draw(canvasSprite);
        }

        m_window.display();

        // Handle close
        if (m_closeRequested)
        {
            m_window.close();
        }
        else if (m_recreateWindow)
        {
            m_recreateWindow = false;
            createWindow();
        }
    }

    // Disconnect on exit
    sConnection.disconnect();

    LOG_INFO("Game loop ended");
}

// Binary: FUN_00405d40
// Polls SFML events. Key events mapped to internal state arrays.
void Application::input()
{
    sUI.resetInputConsumed();

    while (auto event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
            return;
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>())
        {
            onResize(resized->size.x, resized->size.y);
        }

        if (event->is<sf::Event::FocusLost>())
        {
            m_hasFocus = false;
        }

        if (event->is<sf::Event::FocusGained>())
        {
            m_hasFocus = true;
        }

        // Phase 5: Pass events to UI layer
        float mx = static_cast<float>(m_mousePos.x);
        float my = static_cast<float>(m_mousePos.y);
        sUI.handleEvent(*event, mx, my);
    }

    // Frame counter (binary: param_1[0x24] = param_1[0x23]; param_1[0x23]++)
    m_prevFrameCount = m_frameCount;
    m_frameCount++;
}

// Binary: FUN_004592d0 (network update, called via render object chain)
// Polls SfSocket::update() for incoming data, then processes all queued packets.
// Binary also has a 15-minute idle timeout disconnect; not implemented in Phase 1.
void Application::updateNetwork()
{
    if (!sConnection.isConnected())
        return;

    // Poll socket for incoming data and flush sends
    if (!sConnection.update())
    {
        LOG_WARN("Disconnected from server");
        sConnection.disconnect();
        return;
    }

    // Drain received packets and dispatch
    std::vector<ReceivedPacket> packets;
    sConnection.popReceived(packets);
    if (!packets.empty())
    {
        sPacketHandler.processPackets(packets);
    }
}

// Phase 2: Game state progression.
// After auth success, handleValidate auto-requests character list.
// After receiving character list, auto-enter world with first character (testing).
// After receiving SMSG_PLAYER (self), state transitions to InWorld.
void Application::updateGameState()
{
    GameState state = sPacketHandler.getGameState();

    if (state == GameState::CharSelect && !m_enteredWorld)
    {
        const auto& charList = sPacketHandler.getCharacterList();
        if (!charList.empty() && !m_charListReceived)
        {
            m_charListReceived = true;

            // Auto-enter world with last character (most recently created)
            const auto& ch = charList.back();
            LOG_INFO("Auto-entering world with '%s' (guid=%d, level=%d)",
                     ch.name.c_str(), ch.guid, ch.level);
            PacketSender::sendEnterWorld(static_cast<uint32_t>(ch.guid));
        }
        else if (charList.empty() && sPacketHandler.getAuthState() == PacketHandler::AuthState::Authenticated)
        {
            // No characters - create one for testing (only once)
            static bool createdChar = false;
            if (!createdChar)
            {
                createdChar = true;
                LOG_INFO("No characters found, creating test character");
                PacketSender::sendCharCreate("TestHero", 1, 0, 0);  // Paladin, Male, portrait 0
            }
        }
    }

    if (state == GameState::InWorld && !m_enteredWorld)
    {
        m_enteredWorld = true;
        const auto& player = sPacketHandler.getLocalPlayer();
        LOG_INFO("Entered world: '%s' on map %u at (%.1f, %.1f)",
                 player.name.c_str(), player.mapId, player.posX, player.posY);
        LOG_INFO("  Class=%u Level=%u HP=%d/%d Mana=%d/%d XP=%u Gold=%u",
                 player.classId, player.level, player.health, player.maxHealth,
                 player.mana, player.maxMana, player.xp, player.money);

        // Phase 3: Load map and initialize camera
        loadMap(player.mapId);
    }

    // Phase 5: Create HUD widgets once map is loaded
    if (m_mapLoaded && !m_hudCreated)
    {
        m_hudCreated = true;

        auto unitFrame = std::make_unique<UnitFrame>();
        unitFrame->init();
        m_unitFrame = unitFrame.get();
        sUI.addRoot(std::move(unitFrame));

        auto toolbar = std::make_unique<Toolbar>();
        toolbar->init(m_width, m_height);
        m_toolbar = toolbar.get();
        sUI.addRoot(std::move(toolbar));

        auto chatWindow = std::make_unique<ChatWindow>();
        chatWindow->init(m_height);
        m_chatWindow = chatWindow.get();
        sUI.addRoot(std::move(chatWindow));

        // Wire chat messages to ChatWindow
        sPacketHandler.setChatCallback([this](const std::string& msg) {
            if (m_chatWindow)
                m_chatWindow->addMessage(msg);
        });

        LOG_INFO("HUD created");
    }

    // Phase 5: Update UI
    sUI.update(getDeltaSeconds());

    // Phase 5: Update HUD data
    if (m_unitFrame)
        m_unitFrame->updateFromPlayer(sPacketHandler.getLocalPlayer());

    // Phase 4: Process movement input and update world entities
    if (m_mapLoaded && m_clientMap)
    {
        float dt = getDeltaSeconds();
        auto& player = sPacketHandler.getLocalPlayer();

        // Update input manager movement speed from player data
        if (player.moveSpeed > 0.f)
            m_inputManager.setMoveSpeed(player.moveSpeed);

        // Process WASD input (skip if UI consumed mouse this frame)
        bool isMoving = false;
        if (!sUI.wasInputConsumed())
            isMoving = m_inputManager.update(dt);
        else
            isMoving = false;
        if (isMoving)
        {
            // Apply movement delta to local player position
            player.posX += m_inputManager.getDeltaX();
            player.posY += m_inputManager.getDeltaY();
            player.orientation = m_inputManager.getOrientation();

            // Send movement packet (throttled to ~15 per second)
            m_moveSendTimer += dt;
            if (m_moveSendTimer >= 0.066f)
            {
                m_moveSendTimer = 0.f;
                PacketSender::sendRequestMove(player.posX, player.posY, player.orientation);
            }
        }
        else if (m_inputManager.movementChanged())
        {
            // Stopped moving - send final position
            m_moveSendTimer = 0.f;
            PacketSender::sendRequestMove(player.posX, player.posY, player.orientation);
        }

        // Update world entity interpolation
        sWorldState.update(dt);

        // Update sprite animations for all entities
        m_entityRenderer.update(dt, player.guid, isMoving, player.orientation, player.gender);

        // Update camera to follow player position
        m_camera.update(player.posX, player.posY, dt);
        m_clientMap->setCameraPosition(m_camera.getX(), m_camera.getY());
    }
}

// Phase 3: Load map from mapId.
// Binary: World::setMap at 0x00537340
//   - Looks up map name from ContentMgr::db("mp")
//   - Calls GameMap::loadFromDisk(mapname)
//   - Sets camera zoom to 3.0f
//   - Initializes minimap
void Application::loadMap(uint32_t mapId)
{
    MapInfo mapInfo;
    if (!sContentDB.getMapInfo(static_cast<int>(mapId), mapInfo))
    {
        LOG_ERROR("loadMap: map ID %u not found in game.db", mapId);
        return;
    }

    std::string mapFilePath = m_assetPath + "/maps/" + mapInfo.name + ".map";
    LOG_INFO("Loading map: '%s' (id=%u)", mapInfo.name.c_str(), mapId);

    MapFileData mapFileData;
    if (!MapLoader::load(mapFilePath, mapFileData))
    {
        LOG_ERROR("loadMap: failed to parse '%s'", mapFilePath.c_str());
        return;
    }

    m_clientMap = std::make_unique<ClientMap>();
    if (!m_clientMap->loadFromFile(mapFileData))
    {
        LOG_ERROR("loadMap: failed to build ClientMap");
        m_clientMap.reset();
        return;
    }

    // Initialize camera at player spawn position
    m_camera.setScreenCenter(static_cast<float>(m_width) / 2.0f,
                              static_cast<float>(m_height) / 2.0f);
    m_camera.setScreenHeight(static_cast<float>(m_height));

    const auto& player = sPacketHandler.getLocalPlayer();
    m_camera.teleportTo(player.posX, player.posY);
    m_clientMap->setCameraPosition(m_camera.getX(), m_camera.getY());

    m_mapLoaded = true;
    LOG_INFO("Map '%s' loaded and camera initialized at player position (%.1f, %.1f)",
             mapInfo.name.c_str(), player.posX, player.posY);
}

// Binary: FUN_004061a0
// Draws all game content.
// Binary calls FUN_004f3910 here (draw all game objects via render object system).
// Phase 3: Renders the isometric tile map when in world.
void Application::render()
{
    sf::RenderTarget& target = getActiveTarget();

    if (m_mapLoaded && m_clientMap)
    {
        m_mapRenderer.render(target, *m_clientMap, m_width, m_height);

        // Phase 4: Render entities on top of map
        const auto& player = sPacketHandler.getLocalPlayer();
        m_entityRenderer.render(target, m_camera.getX(), m_camera.getY(),
                                m_width, m_height,
                                player.guid, player.posX, player.posY,
                                player.gender);
    }

    // Phase 5: Render UI on top of everything
    sUI.render(target);
}

// Binary: FUN_004088f0
void Application::toggleFullscreen(bool fullscreen)
{
    if (m_fullscreen == fullscreen)
        return;

    m_fullscreen = fullscreen;
    sConfig->setInt("Window", "Fullscreen", fullscreen ? 1 : 0);
    m_recreateWindow = true;
}

// Binary: FUN_00407d20
void Application::onResize(unsigned int newWidth, unsigned int newHeight)
{
    if (newWidth < 1280) newWidth = 1280;
    if (newHeight < 720) newHeight = 720;

    m_width  = newWidth;
    m_height = newHeight;

    sConfig->setInt("Window", "Width", static_cast<int>(newWidth));
    sConfig->setInt("Window", "Height", static_cast<int>(newHeight));

    // Recreate canvas at new size
    if (m_useCanvas)
    {
        (void)m_canvas.resize({newWidth, newHeight});
    }

    // Update the view
    sf::View view(sf::FloatRect({0.f, 0.f}, {static_cast<float>(newWidth), static_cast<float>(newHeight)}));
    getActiveTarget().setView(view);
}
