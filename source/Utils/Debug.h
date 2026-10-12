#pragma once

#include <windows.h>
#include "RainmeterAPI.h"

#include <string>
#include <vector>

extern void* g_Rainmeter;

// Always visible — module/provider startup events and errors. No debug level gate.
void LogAlways(int level, const wchar_t* fmt, ...);

// Every LogAlways line is also kept here, capped and in order. A Debug=2 dump prints it,
// so the file a reporter sends carries the provider story with it. Those lines are written
// once, when a provider first initialises, so by the time anyone thinks to look at the log
// they have usually scrolled away - which is exactly what happened with the PMLog line on
// the forum. Safe to call from any thread.
std::vector<std::wstring> StartupLog();

// Per-measure — only visible when the measure's own Debug >= 1.
// Pass the measure's rm handle and its debugLevel directly.
void LogForMeasure(void* rm, int measureDebugLevel, const wchar_t* fmt, ...);

#define LOG_STARTUP(fmt, ...)               LogAlways(LOG_NOTICE, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                 LogAlways(LOG_ERROR,  L"[%S:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_MEASURE(rm, lvl, fmt, ...)      LogForMeasure(rm, lvl, fmt, ##__VA_ARGS__)
