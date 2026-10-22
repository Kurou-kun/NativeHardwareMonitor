#pragma once

#include <cstdint>
#include <cwctype>

#include "Types/Category.h"
#include "Types/GpuMetric.h"
#include "Types/StorageMetric.h"

// TemperatureUnit=C|F|K (issue #16). Every provider publishes Celsius and keeps doing so; the
// conversion happens once, on the way out to Rainmeter, so no provider has to know about it.
// A hub measure's setting also covers the section variables it serves.

enum class TemperatureUnit { Celsius, Fahrenheit, Kelvin, Invalid };

// Absent or empty means Celsius. Otherwise exactly one letter, either case - anything else is
// Invalid, and the measure is disabled rather than guessing what "Farenheit" or "1" meant.
inline TemperatureUnit ParseTemperatureUnit(const wchar_t* text)
{
    if (!text || !text[0])
        return TemperatureUnit::Celsius;
    if (text[1])
        return TemperatureUnit::Invalid;

    switch (towlower(text[0]))
    {
    case L'c': return TemperatureUnit::Celsius;
    case L'f': return TemperatureUnit::Fahrenheit;
    case L'k': return TemperatureUnit::Kelvin;
    default:   return TemperatureUnit::Invalid;
    }
}

inline bool IsTemperatureMetric(Category category, uint32_t metric)
{
    if (category == Category::GPU)
    {
        switch (static_cast<GpuMetric>(metric))
        {
        case GpuMetric::Temperature:
        case GpuMetric::HotspotTemperature:
        case GpuMetric::MemoryTemperature:
        case GpuMetric::IntakeTemperature:
            return true;
        default:
            return false;
        }
    }

    return category == Category::Storage && metric == static_cast<uint32_t>(StorageMetric::Temperature);
}

// -1 (unsupported) and -2 (error) are not measurements. Converted, -1 would become 30.2 F or
// 272.15 K: a believable temperature from a sensor that does not exist, which is the whole
// reason this lives in the plugin instead of a Calc.
inline double ConvertTemperature(double celsius, TemperatureUnit unit)
{
    if (celsius == -1.0 || celsius == -2.0)
        return celsius;

    switch (unit)
    {
    case TemperatureUnit::Fahrenheit: return celsius * 1.8 + 32.0;
    case TemperatureUnit::Kelvin:     return celsius + 273.15;
    default:                          return celsius;
    }
}
