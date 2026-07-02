#include "Utils/Debug.h"

#include <cstdarg>
#include <cstdio>

void* g_Rainmeter  = nullptr;
int   g_DebugLevel = 0;

static void DoLog(void* rm, int level, const wchar_t* fmt, va_list args)
{
    wchar_t msg[1024];
    wchar_t buf[1100];
    vswprintf(msg, 1024, fmt, args);
    swprintf(buf, 1100, L"[NHM] %s", msg);
    RmLog(rm, level, buf);
}

void LogAlways(int level, const wchar_t* fmt, ...)
{
    if (!g_Rainmeter)
        return;

    va_list args;
    va_start(args, fmt);
    DoLog(g_Rainmeter, level, fmt, args);
    va_end(args);
}

void LogForMeasure(void* rm, int measureDebugLevel, const wchar_t* fmt, ...)
{
    if (!rm || measureDebugLevel < 1)
        return;

    va_list args;
    va_start(args, fmt);
    DoLog(rm, LOG_NOTICE, fmt, args);
    va_end(args);
}
