#pragma once

#include <windows.h>
#include "RainmeterAPI.h"

extern void* g_Rainmeter;
extern int   g_DebugLevel;

// Always visible — module/provider startup events and errors. No debug level gate.
void LogAlways(int level, const wchar_t* fmt, ...);

// Per-measure — only visible when the measure's own Debug >= 1.
// Pass the measure's rm handle and its debugLevel directly.
void LogForMeasure(void* rm, int measureDebugLevel, const wchar_t* fmt, ...);

#define LOG_STARTUP(fmt, ...)               LogAlways(LOG_NOTICE, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                 LogAlways(LOG_ERROR,  L"[%S:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_MEASURE(rm, lvl, fmt, ...)      LogForMeasure(rm, lvl, fmt, ##__VA_ARGS__)
