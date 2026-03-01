#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

#include "map/ClientMap.h"
#include "map/Camera.h"
#include "map/MapRenderer.h"
#include "input/InputManager.h"
#include "render/EntityRenderer.h"

// Forward declarations for HUD widgets
class UnitFrame;
class Toolbar;
class ChatWindow;

// Application class - singleton game shell.
// Decompiled from binary at 0x00405110 (constructor), 0x00405650 (run),
// 0x00405d40 (input), 0x004061a0 (render), 0x00407f10 (createSfmlWindow).
//
// Global instance at 0x00682a20 in the binary.
// Singleton accessor at 0x00403ea0.
//
// Network flow (from binary decompilation):
//   Binary: network update happens inside render path via render object updates.
//     FUN_004061a0 (render) -> FUN_004f3910 (update render objects) -> FUN_004592d0 (network update)
//   FUN_004592d0: calls SfSocket::update(), then drains packet queue -> processPacket.
//   Login flow (FUN_0049d5e0): HTTP auth -> TCP connect -> send GP_Client_Authenticate.
//   Phase 1: simplified - direct TCP connect + auth, network update in main loop.

class Application
{
public:
    static Application& instance();

    void run();

    // Window access
    sf::RenderWindow& getWindow() { return m_window; }
    sf::RenderTexture& getCanvas() { return m_canvas; }
    sf::RenderTarget& getActiveTarget();

    // State flags
    bool hasFocus() const { return m_hasFocus; }
    bool isDevMode() const { return m_devMode; }
    bool isFullscreen() const { return m_fullscreen; }
    bool isOriginalDPI() const { return m_originalDPI; }

    // Window dimensions (the configured/saved dimensions)
    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }

    // Delta time for current frame
    float getDeltaSeconds() const { return m_deltaTime.asSeconds(); }
    sf::Time getDeltaTime() const { return m_deltaTime; }

    // Mouse position (updated each frame)
    sf::Vector2i getMousePosition() const { return m_mousePos; }

    // Frame counter
    uint32_t getFrameCount() const { return m_frameCount; }

    // Request close
    void requestClose() { m_closeRequested = true; }

    // Fullscreen toggle (binary: FUN_004088f0)
    void toggleFullscreen(bool fullscreen);

    // Resize handler (binary: FUN_00407d20)
    void onResize(unsigned int newWidth, unsigned int newHeight);

private:
    Application();
    ~Application() = default;

    void initConfig();
    void initResources();
    void createWindow();
    void showLoadingScreen();
    void input();
    void updateNetwork();
    void updateGameState();
    void loadMap(uint32_t mapId);
    void render();

    // Window
    sf::RenderWindow  m_window;
    sf::RenderTexture m_canvas;
    bool m_useCanvas = true;   // false when OriginalDPI mode

    // State
    bool m_hasFocus        = true;
    bool m_closeRequested  = false;
    bool m_recreateWindow  = false;
    bool m_originalDPI     = false;
    bool m_fullscreen      = false;
    bool m_devMode         = false;

    // Timing
    sf::Clock m_clock;
    sf::Time  m_deltaTime;

    // Input
    sf::Vector2i m_mousePos;

    // Frame tracking
    uint32_t m_frameCount    = 0;
    uint32_t m_prevFrameCount = 0;

    // Configured window dimensions
    unsigned int m_width  = 1280;
    unsigned int m_height = 720;
    unsigned int m_maxFps = 300;

    // Saved window position for fullscreen toggle
    sf::Vector2i m_savedPosition;
    unsigned int m_savedWidth  = 0;
    unsigned int m_savedHeight = 0;

    // Window title
    std::string m_windowName = "Dreadmyst";

    // Game state tracking for auto-flow (Phase 2)
    bool m_charListReceived  = false;
    bool m_enteredWorld      = false;

    // Phase 3: Map + Camera + Renderer
    std::unique_ptr<ClientMap> m_clientMap;
    Camera m_camera;
    MapRenderer m_mapRenderer;
    bool m_mapLoaded = false;

    // Phase 4: Input + Entity rendering
    InputManager m_inputManager;
    EntityRenderer m_entityRenderer;
    float m_moveSendTimer = 0.f;  // throttle movement packets

    // Asset base path
    std::string m_assetPath;

    // Phase 5: HUD widget references (owned by UIManager)
    UnitFrame*  m_unitFrame  = nullptr;
    Toolbar*    m_toolbar    = nullptr;
    ChatWindow* m_chatWindow = nullptr;
    bool m_hudCreated = false;
};

#define sApp Application::instance()
