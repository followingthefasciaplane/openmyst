#pragma once

#include <string>
#include <cstdio>
#include <cstdarg>

// Simple logger used by client and shared code
namespace Logger
{
    enum class Level
    {
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
    };

    inline void log(Level level, const char* fmt, ...)
    {
        const char* prefix = "";
        switch (level) {
            case Level::Debug:   prefix = "[DEBUG] "; break;
            case Level::Info:    prefix = "[INFO]  "; break;
            case Level::Warning: prefix = "[WARN]  "; break;
            case Level::Error:   prefix = "[ERROR] "; break;
            case Level::Fatal:   prefix = "[FATAL] "; break;
        }
        fprintf(stderr, "%s", prefix);
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

#define LOG_DEBUG(...)   Logger::log(Logger::Level::Debug, __VA_ARGS__)
#define LOG_INFO(...)    Logger::log(Logger::Level::Info, __VA_ARGS__)
#define LOG_WARNING(...) Logger::log(Logger::Level::Warning, __VA_ARGS__)
#define LOG_ERROR(...)   Logger::log(Logger::Level::Error, __VA_ARGS__)
#define LOG_FATAL(...)   Logger::log(Logger::Level::Fatal, __VA_ARGS__)
