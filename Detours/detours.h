#pragma once

// Microsoft Detours library stub
// Used for hooking functions in the client binary (development/modding tools)
// The actual implementation is the MS Detours library

#include <windows.h>

// Detours transaction API
LONG DetourTransactionBegin();
LONG DetourTransactionCommit();
LONG DetourUpdateThread(HANDLE hThread);
LONG DetourAttach(PVOID* ppPointer, PVOID pDetour);
LONG DetourDetach(PVOID* ppPointer, PVOID pDetour);
