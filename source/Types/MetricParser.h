#pragma once

#include <string>
#include "Types/Category.h"

Category ParseCategory(const std::wstring& input);
uint32_t ParseMetric(Category category, const std::wstring& input);

// Reverse/introspection helpers used by the Debug=2 dump.
std::wstring CategoryName(Category category);          // e.g. L"GPU"
uint32_t     MetricCount(Category category);           // number of real metrics (== the enum's Unknown value)
std::wstring MetricName(Category category, uint32_t metricId); // canonical name, or L"Metric[<id>]" fallback
