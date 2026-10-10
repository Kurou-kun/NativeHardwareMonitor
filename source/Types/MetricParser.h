#pragma once

#include <string>
#include "Types/Category.h"

// ParseMetric returns this for "Count"/"DeviceCount" in any category. The device
// count is the same question everywhere, so HardwareCore answers it from
// IModule::GetDeviceCount() instead of every module carrying a metric of its own
// for it. Deliberately distinct from -1 (unknown category) and far outside the
// range of any category's metric enum.
static constexpr uint32_t METRIC_DEVICE_COUNT = 0xFFFFFFFEu;

Category ParseCategory(const std::wstring& input);
uint32_t ParseMetric(Category category, const std::wstring& input);

// Reverse/introspection helpers used by the Debug=2 dump.
std::wstring CategoryName(Category category);          // e.g. L"GPU"
uint32_t     MetricCount(Category category);           // number of real metrics (== the enum's Unknown value)
std::wstring MetricName(Category category, uint32_t metricId); // canonical name, or L"Metric[<id>]" fallback
