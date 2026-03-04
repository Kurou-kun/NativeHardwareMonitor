#pragma once

#include <cstdint>
#include "Types/Category.h"

struct MeasureContext
{
    bool valid = false;

    Category category = Category::Unknown;
    uint32_t metricId = 0;
    uint32_t deviceIndex = 0;

    uint32_t handle = 0;
};