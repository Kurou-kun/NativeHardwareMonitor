// Pins FormatSectionValue, the whole formatting surface a section variable has.
//
// The `auto` expectations below are NOT read off our own implementation - they are what
// Rainmeter's AutoScale=1 / =2 print for the same numbers (Library/Measure.cpp,
// GetScaledValue), trailing space and all. If a future "tidy-up" drops the space before
// the unit letter, or scales a -1 sentinel, this stops compiling clean and starts failing.
//
// Build and run (no framework, no project entry, no Windows dependency):
//   cl /EHsc /std:c++17 /I source tools\section_format_check.cpp

#include "Utils/ValueFormat.h"

#include <cassert>
#include <cstdio>
#include <string>

static int g_checks = 0;

static void Expect(double value, const wchar_t* decimals, const wchar_t* scale, const wchar_t* want)
{
    std::wstring got;
    FormatSectionValue(value, decimals, scale, got);

    if (got != want)
    {
        wprintf(L"FAIL  value=%.6f decimals='%s' scale='%s'  got '%s', want '%s'\n",
            value, decimals ? decimals : L"(null)", scale ? scale : L"(null)",
            got.c_str(), want);
        assert(false && "FormatSectionValue mismatch");
    }

    ++g_checks;
}

int main()
{
    // --- no arguments: three decimals, trailing zeros trimmed, whole numbers left whole
    Expect(0.0,          nullptr, nullptr, L"0");
    Expect(42.0,         nullptr, nullptr, L"42");
    Expect(42.5,         nullptr, nullptr, L"42.5");
    Expect(42.123456,    nullptr, nullptr, L"42.123");
    Expect(-1.0,         nullptr, nullptr, L"-1");
    Expect(86021914624.0, nullptr, nullptr, L"86021914624");  // the raw byte count from #10
    Expect(3400000000.0, nullptr, nullptr, L"3400000000");    // Hz, not MHz

    // An omitted middle argument arrives as an empty string, not as a missing one
    Expect(42.5, L"", L"", L"42.5");

    // --- decimals
    Expect(42.567, L"0",  nullptr, L"43");
    Expect(42.567, L"1",  nullptr, L"42.6");
    Expect(42.0,   L"2",  nullptr, L"42.00");   // explicit decimals keep their zeros
    Expect(42.567, L"-3", nullptr, L"43");      // clamped to 0, never negative
    Expect(1.0,    L"99", nullptr, L"1.000000000000000");  // clamped to 15

    // --- scale as a divisor, with and without Rainmeter's leading slash
    Expect(86021914624.0, L"1", L"1073741824",  L"80.1");
    Expect(86021914624.0, L"1", L"/1073741824", L"80.1");
    Expect(3400000000.0,  L"0", L"/1000000",    L"3400");
    Expect(86021914624.0, nullptr, L"/1073741824", L"80.114");  // default decimals still apply

    // A typo prints the raw value rather than a lie, and never divides by zero
    Expect(1024.0, L"0", L"banana", L"1024");
    Expect(1024.0, L"0", L"0",      L"1024");
    Expect(1024.0, L"0", L"/0",     L"1024");

    // --- auto: must match AutoScale=1 exactly, including the space before the unit and
    //     the lone trailing space when nothing was big enough to scale
    Expect(512.0,            L"1", L"auto", L"512.0 ");
    Expect(1024.0,           L"1", L"auto", L"1.0 k");
    Expect(1048576.0,        L"1", L"auto", L"1.0 M");
    Expect(86021914624.0,    L"1", L"auto", L"80.1 G");
    Expect(1099511627776.0,  L"1", L"auto", L"1.0 T");
    Expect(1023.0,           L"0", L"auto", L"1023 ");      // one byte short of kilo
    Expect(86021914624.0,    L"1", L"AUTO", L"80.1 G");     // argument is case-insensitive
    Expect(86021914624.0,    L"1", L"auto1024", L"80.1 G"); // explicit base
    Expect(86021914624.0,    L"1", L"auto1000", L"86.0 G"); // AutoScale=2
    Expect(1000.0,           L"1", L"auto1000", L"1.0 k");
    Expect(1000.0,           L"1", L"auto",     L"1000.0 ");// 1000 bytes is not a kilobyte

    // --- a sentinel is never scaled: -1 has to stay testable whatever the skin passes
    Expect(-1.0, nullptr, L"auto",       L"-1");
    Expect(-1.0, L"0",    L"auto1000",   L"-1");
    Expect(-1.0, L"0",    L"/1073741824", L"-1");
    Expect(-2.0, nullptr, L"auto",       L"-2");  // the wrong-Category reading

    wprintf(L"section_format_check: %d assertions passed\n", g_checks);
    return 0;
}
