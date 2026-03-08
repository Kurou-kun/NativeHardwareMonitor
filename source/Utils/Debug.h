#pragma once

#include <windows.h>
#include "RainmeterAPI.h"

// Global Rainmeter handle for logging
extern void* g_Rainmeter;

// Global debug switch
extern bool g_DebugEnabled;

// Logging function
void DebugLog(int level, const wchar_t* fmt, ...);

// INFO logs (only when Debug=1)
#define LOG_INFO(fmt, ...) \
    DebugLog(LOG_NOTICE, fmt, ##__VA_ARGS__)

// ERROR logs (always shown, with file + line)
#define LOG_ERROR(fmt, ...) \
    DebugLog(LOG_ERROR, L"%S:%d | " fmt, __FILE__, __LINE__, ##__VA_ARGS__)