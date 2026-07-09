#pragma once

#include <cstdint>
#include <string>
#include "Types/Category.h"

struct MeasureContext
{
    bool valid = false;

    Category  category         = Category::Unknown;
    uint32_t  metricId         = 0;
    uint32_t  deviceIndex      = 0;
    uint32_t  handle           = 0;
    int       debugLevel       = 0;
    uint32_t  updateOverrideMs = 0;
    uint32_t  pingIntervalMs   = 1000;
    uint64_t  lastDumpMs       = 0; // Debug=2 dump throttle

    std::wstring categoryStr;
    std::wstring metricStr;
    std::wstring host;
};
