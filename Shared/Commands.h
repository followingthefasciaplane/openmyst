#pragma once

#include <string>
#include <functional>
#include <map>

// Client-side slash commands (e.g., /whisper, /guild, /dance)
namespace Commands
{
    // Command handler signature
    using Handler = std::function<void(const std::string& args)>;

    // Register a slash command
    void registerCommand(const std::string& name, Handler handler);

    // Process a chat line - returns true if it was a command
    bool processCommand(const std::string& input);
}
