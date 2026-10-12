#include "Utils/Debug.h"

#include <cstdarg>
#include <cstdio>
#include <mutex>

void* g_Rainmeter = nullptr;

static void DoLog(void* rm, int level, const wchar_t* fmt, va_list args)
{
    wchar_t msg[1024];
    wchar_t buf[1100];
    vswprintf(msg, 1024, fmt, args);
    swprintf(buf, 1100, L"[NHM] %s", msg);
    RmLog(rm, level, buf);
}

// Providers initialise on a poller thread, so this is written from several threads and
// read from Rainmeter's. 64 lines is far more than the ten modules can produce at startup;
// past that the oldest go, since a flood would be repeated errors rather than new facts.
static std::mutex                g_startupMutex;
static std::vector<std::wstring> g_startupLines;

void LogAlways(int level, const wchar_t* fmt, ...)
{
    wchar_t msg[1024];
    {
        va_list args;
        va_start(args, fmt);
        vswprintf(msg, 1024, fmt, args);
        va_end(args);
    }

    {
        std::lock_guard<std::mutex> lock(g_startupMutex);
        if (g_startupLines.size() >= 64)
            g_startupLines.erase(g_startupLines.begin());
        g_startupLines.emplace_back(msg);
    }

    // Kept even with no Rainmeter handle yet: the record matters more than the log line.
    if (!g_Rainmeter)
        return;

    wchar_t buf[1100];
    swprintf(buf, 1100, L"[NHM] %s", msg);
    RmLog(g_Rainmeter, level, buf);
}

std::vector<std::wstring> StartupLog()
{
    std::lock_guard<std::mutex> lock(g_startupMutex);
    return g_startupLines;
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
