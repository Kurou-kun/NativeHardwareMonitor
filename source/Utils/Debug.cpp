#include "Utils/Debug.h"
#include <cstdarg>
#include <cstdio>

void* g_Rainmeter = nullptr;
bool g_DebugEnabled = false;

void DebugLog(int level, const wchar_t* fmt, ...)
{
    if (!g_Rainmeter)
        return;

    // Only show INFO logs when debug is enabled
    if (!g_DebugEnabled && level == LOG_NOTICE)
        return;

    wchar_t message[1024];
    wchar_t buffer[1100];

    va_list args;
    va_start(args, fmt);
    vswprintf(message, 1024, fmt, args);
    va_end(args);

    swprintf(buffer, 1100, L"[NHM] %s", message);

    RmLog(g_Rainmeter, level, buffer);
}