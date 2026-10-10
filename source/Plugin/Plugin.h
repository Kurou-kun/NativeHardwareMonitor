#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Types/Category.h"

// Stored in SectionHandle::handle when the metric name in a section variable
// failed to parse: the error is logged once and later reads return the
// unsupported sentinel instead of logging it again every update.
static constexpr uint32_t SV_BAD_METRIC = 0xFFFFFFFFu;

// One lazily registered HardwareCore handle behind a section variable call.
// Section variables name their metric per call ([MeasureName:GPU(temp)]), so a
// hub measure accumulates one of these per distinct category/metric/device
// triple it is asked for and releases them all in Finalize.
struct SectionHandle
{
    Category category = Category::Unknown;
    uint32_t metric   = 0;
    uint32_t device   = 0;
    uint32_t handle   = 0;
};

struct MeasureContext
{
    bool valid = false;
    bool hub   = false; // no Category= at all: the measure only hosts section variables

    Category  category         = Category::Unknown;
    uint32_t  metricId         = 0;
    uint32_t  deviceIndex      = 0;
    uint32_t  handle           = 0;
    int       debugLevel       = 0;
    uint32_t  updateOverrideMs = 0;
    uint32_t  pingIntervalMs   = 1000;

    std::wstring categoryStr;
    std::wstring metricStr;
    std::wstring host;

    std::vector<SectionHandle> svHandles;
    std::wstring               svBuffer; // backs the LPCWSTR handed to Rainmeter
};
