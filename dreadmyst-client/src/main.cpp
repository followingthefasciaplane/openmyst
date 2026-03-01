#include "core/Application.h"
#include "core/Log.h"
#include "DmystVersion.h"

// Entry point for Dreadmyst Client.
// Binary entry: FUN_004a86e0 (WinMain equivalent).
//
// Original binary flow:
//   1. OpenGL context check (GPU capabilities)
//   2. timeBeginPeriod(1) (Windows timer resolution)
//   3. SetCurrentDirectory("..\\")
//   4. Sentry crash reporting init
//   5. Steam API init (required in original)
//   6. Application::getInstance()->run()
//   7. SteamAPI_Shutdown()
//
// We skip Steam/Sentry for the reimplementation.

int main(int /*argc*/, char* /*argv*/[])
{
    LOG_INFO("Dreadmyst Client r%d starting", DMYST_REVISION);

    Application& app = Application::instance();
    app.run();

    LOG_INFO("Dreadmyst Client shutdown complete");
    return 0;
}
