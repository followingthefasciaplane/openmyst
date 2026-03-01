#pragma once
#include <cstdio>
#include <cstdarg>
#include <string>
#include <mutex>

// Logging system matching the original ServerLog at 0x004042a0
// printf-style logging to stdout with optional file output
namespace Log {
    enum Level { LOG_DEBUG = 0, LOG_INFO = 100, LOG_WARN = 200, LOG_ERROR = 300, LOG_FATAL = 400 };

    void init(const std::string& logFile = "");
    void setLevel(Level level);
    void write(Level level, const char* fmt, ...);
    void flush();
}

#define LOG_DEBUG(fmt, ...) Log::write(Log::LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  Log::write(Log::LOG_INFO,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Log::write(Log::LOG_WARN,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Log::write(Log::LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) Log::write(Log::LOG_FATAL, fmt, ##__VA_ARGS__)
