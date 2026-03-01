#pragma once

#include <string>
#include <cstdio>
#include <cstdarg>

// Logging system matching binary's FUN_00414630 behavior.
// The binary uses printf-style logging with severity levels.
// Severity thresholds observed: 100 (error), 400 (perf warning), 200 (info).

enum class LogLevel : int
{
    Error   = 100,
    Info    = 200,
    Warning = 400,
    Debug   = 500,
};

class Log
{
public:
    static void write(LogLevel level, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);

        const char* prefix = "";
        switch (level)
        {
            case LogLevel::Error:   prefix = "[ERROR] ";   break;
            case LogLevel::Info:    prefix = "[INFO] ";    break;
            case LogLevel::Warning: prefix = "[WARN] ";    break;
            case LogLevel::Debug:   prefix = "[DEBUG] ";   break;
        }

        std::fprintf(stdout, "%s", prefix);
        std::vfprintf(stdout, fmt, args);
        std::fprintf(stdout, "\n");
        std::fflush(stdout);

        va_end(args);
    }

    static void error(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::fprintf(stderr, "[ERROR] ");
        std::vfprintf(stderr, fmt, args);
        std::fprintf(stderr, "\n");
        std::fflush(stderr);
        va_end(args);
    }

    static void info(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::fprintf(stdout, "[INFO] ");
        std::vfprintf(stdout, fmt, args);
        std::fprintf(stdout, "\n");
        std::fflush(stdout);
        va_end(args);
    }

    static void warn(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::fprintf(stdout, "[WARN] ");
        std::vfprintf(stdout, fmt, args);
        std::fprintf(stdout, "\n");
        std::fflush(stdout);
        va_end(args);
    }
};

#define LOG_ERROR(fmt, ...) Log::error(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  Log::info(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Log::warn(fmt, ##__VA_ARGS__)
