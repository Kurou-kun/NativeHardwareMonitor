// Pins FormatSectionValue, which is the whole of what a section variable prints.
//
// Small surface, easy to "improve" wrongly: the trailing-zero trim, the clamp on the
// decimals argument, and the rule that a raw byte count never turns into exponent form or
// gets dressed up with a unit. Scaling deliberately does not live here - see ValueFormat.h.
//
// Build and run (no framework, no project entry, no Windows dependency):
//   cl /EHsc /std:c++17 /I source tools\section_format_check.cpp

#include "Utils/ValueFormat.h"

#include <cassert>
#include <cstdio>
#include <string>

static int g_checks = 0;

static void Expect(double value, const wchar_t* decimals, const wchar_t* want)
{
    std::wstring got;
    FormatSectionValue(value, decimals, got);

    if (got != want)
    {
        wprintf(L"FAIL  value=%.6f decimals='%s'  got '%s', want '%s'\n",
            value, decimals ? decimals : L"(null)", got.c_str(), want);
        assert(false && "FormatSectionValue mismatch");
    }

    ++g_checks;
}

int main()
{
    // --- no argument: three decimals, trailing zeros trimmed, whole numbers left whole
    Expect(0.0,           nullptr, L"0");
    Expect(42.0,          nullptr, L"42");
    Expect(42.5,          nullptr, L"42.5");
    Expect(42.50,         nullptr, L"42.5");
    Expect(42.123456,     nullptr, L"42.123");
    Expect(42.1,          nullptr, L"42.1");

    // The two numbers the bare-number decision is actually about: a byte count and a clock
    // arrive in raw base units, exact, and never in exponent form.
    Expect(86021914624.0, nullptr, L"86021914624");
    Expect(3400000000.0,  nullptr, L"3400000000");

    // Sentinels are not measurements and must survive unchanged
    Expect(-1.0, nullptr, L"-1");
    Expect(-2.0, nullptr, L"-2");
    Expect(-1.0, L"3",    L"-1.000");

    // An omitted middle argument arrives as an empty string, not as a missing one
    Expect(42.5, L"", L"42.5");

    // --- explicit decimals
    Expect(42.567, L"0",  L"43");
    Expect(42.567, L"1",  L"42.6");
    Expect(42.0,   L"2",  L"42.00");   // explicit decimals keep their zeros
    Expect(42.567, L"-3", L"43");      // clamped to 0, never negative
    Expect(1.0,    L"99", L"1.000000000000000");  // clamped to 15

    // ,,15 is the documented way to feed a formula or Lua without losing digits
    Expect(13.690476190476, L"15", L"13.690476190476000");

    wprintf(L"section_format_check: %d assertions passed\n", g_checks);
    return 0;
}
