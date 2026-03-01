#pragma once

#include <windows.h>
#include <string>

namespace CrashHandler
{
    // Install crash handler for SEH and C++ exceptions
    void install();

    // Set crash dump path
    void setDumpPath(const std::string& path);

    // Generate a minidump on unhandled exception
    LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);
}
