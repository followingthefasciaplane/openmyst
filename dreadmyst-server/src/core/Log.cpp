#include "core/Log.h"
#include <ctime>
#include <chrono>

namespace {
    std::mutex g_logMutex;
    FILE* g_logFile = nullptr;
    Log::Level g_minLevel = Log::LOG_INFO;
}

void Log::init(const std::string& logFile) {
    if (!logFile.empty()) {
        g_logFile = fopen(logFile.c_str(), "a");
    }
}

void Log::setLevel(Level level) {
    g_minLevel = level;
}

void Log::write(Level level, const char* fmt, ...) {
    if (level < g_minLevel) return;

    std::lock_guard<std::mutex> lock(g_logMutex);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif

    const char* levelStr = "???";
    switch (level) {
        case LOG_DEBUG: levelStr = "DBG"; break;
        case LOG_INFO:  levelStr = "INF"; break;
        case LOG_WARN:  levelStr = "WRN"; break;
        case LOG_ERROR: levelStr = "ERR"; break;
        case LOG_FATAL: levelStr = "FTL"; break;
    }

    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "[%02d:%02d:%02d.%03d][%s] ",
             tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (int)ms.count(), levelStr);

    va_list args;
    va_start(args, fmt);

    printf("%s", timeBuf);
    vprintf(fmt, args);
    printf("\n");

    if (g_logFile) {
        fprintf(g_logFile, "%s", timeBuf);
        vfprintf(g_logFile, fmt, args);
        fprintf(g_logFile, "\n");
        fflush(g_logFile);
    }

    va_end(args);
}

void Log::flush() {
    if (g_logFile) fflush(g_logFile);
    fflush(stdout);
}
