// Pins TemperatureUnit.h (issue #16): which option values are accepted, which metrics count as
// temperatures, and that the conversion leaves the -1 / -2 sentinels alone - a converted -1
// reads as a real 30.2 F.
//
// Build and run (no framework, no project entry, no Windows dependency):
//   cl /EHsc /std:c++17 /I source tools\temperature_unit_check.cpp

#include "Utils/TemperatureUnit.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>

static int g_checks = 0;

static void ExpectUnit(const wchar_t* text, TemperatureUnit want)
{
    if (ParseTemperatureUnit(text) != want)
    {
        wprintf(L"FAIL  TemperatureUnit='%s' parsed to %d, want %d\n", text ? text : L"(null)",
            static_cast<int>(ParseTemperatureUnit(text)), static_cast<int>(want));
        assert(false && "ParseTemperatureUnit mismatch");
    }
    ++g_checks;
}

static void ExpectConvert(double celsius, TemperatureUnit unit, double want)
{
    double got = ConvertTemperature(celsius, unit);
    if (std::fabs(got - want) > 1e-9)
    {
        printf("FAIL  %.3f C unit %d -> got %.6f, want %.6f\n", celsius, static_cast<int>(unit), got, want);
        assert(false && "ConvertTemperature mismatch");
    }
    ++g_checks;
}

static void ExpectTemp(Category category, uint32_t metric, bool want)
{
    if (IsTemperatureMetric(category, metric) != want)
    {
        printf("FAIL  category %d metric %u: want %s\n", static_cast<int>(category), metric,
            want ? "temperature" : "not a temperature");
        assert(false && "IsTemperatureMetric mismatch");
    }
    ++g_checks;
}

template <typename E>
static uint32_t Id(E e) { return static_cast<uint32_t>(e); }

int main()
{
    using U = TemperatureUnit;

    // --- parsing: one letter, either case; absent/empty is the default
    ExpectUnit(nullptr, U::Celsius);
    ExpectUnit(L"",     U::Celsius);
    ExpectUnit(L"C",    U::Celsius);
    ExpectUnit(L"c",    U::Celsius);
    ExpectUnit(L"F",    U::Fahrenheit);
    ExpectUnit(L"f",    U::Fahrenheit);
    ExpectUnit(L"K",    U::Kelvin);
    ExpectUnit(L"k",    U::Kelvin);
    // anything else disables the measure rather than guessing
    ExpectUnit(L"X",          U::Invalid);
    ExpectUnit(L"1",          U::Invalid);
    ExpectUnit(L"Fahrenheit", U::Invalid);
    ExpectUnit(L"FF",         U::Invalid);
    ExpectUnit(L" F",         U::Invalid);

    // --- conversion
    ExpectConvert(0.0,   U::Celsius,    0.0);
    ExpectConvert(41.5,  U::Celsius,    41.5);
    ExpectConvert(0.0,   U::Fahrenheit, 32.0);
    ExpectConvert(100.0, U::Fahrenheit, 212.0);
    ExpectConvert(-40.0, U::Fahrenheit, -40.0);  // where the scales meet
    ExpectConvert(41.5,  U::Fahrenheit, 106.7);
    ExpectConvert(0.0,   U::Kelvin,     273.15);
    ExpectConvert(41.5,  U::Kelvin,     314.65);
    // sentinels pass through in every unit
    for (U u : { U::Celsius, U::Fahrenheit, U::Kelvin })
    {
        ExpectConvert(-1.0, u, -1.0);
        ExpectConvert(-2.0, u, -2.0);
    }

    // --- the five temperature metrics
    ExpectTemp(Category::GPU,     Id(GpuMetric::Temperature),        true);
    ExpectTemp(Category::GPU,     Id(GpuMetric::HotspotTemperature), true);
    ExpectTemp(Category::GPU,     Id(GpuMetric::MemoryTemperature),  true);
    ExpectTemp(Category::GPU,     Id(GpuMetric::IntakeTemperature),  true);
    ExpectTemp(Category::Storage, Id(StorageMetric::Temperature),    true);

    // --- neighbours that must stay untouched
    ExpectTemp(Category::GPU,     Id(GpuMetric::CoreClock),          false);
    ExpectTemp(Category::GPU,     Id(GpuMetric::VramUsed),           false);
    ExpectTemp(Category::Storage, Id(StorageMetric::ReadBytes),      false);
    ExpectTemp(Category::Storage, Id(StorageMetric::VolumeLabel),    false);
    // Same numeric id as a temperature, wrong category: the category must decide.
    ExpectTemp(Category::CPU,     Id(GpuMetric::Temperature),        false);
    ExpectTemp(Category::Memory,  Id(StorageMetric::Temperature),    false);
    // Reserved ids answered before any table lookup (count / provider status).
    ExpectTemp(Category::GPU,     0xFFFFFFFEu,                       false);
    ExpectTemp(Category::Storage, 0xFFFFFFFDu,                       false);

    printf("temperature_unit_check: %d assertions passed\n", g_checks);
    return 0;
}
