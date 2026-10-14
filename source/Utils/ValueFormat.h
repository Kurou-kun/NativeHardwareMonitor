#pragma once

#include <string>

#include <cstdio>
#include <cwchar>

// Formatting a section variable happens HERE, in the plugin, and that is not a design
// preference - it is the only place it can happen. Rainmeter's own parser settles it
// (Library/ConfigParser.cpp, GetSectionVariable): when the text after the colon names a
// plugin function, the selector goes straight to MeasurePlugin::CommandWithReturn and
// whatever string comes back is the substitution, full stop. The modifiers Rainmeter
// documents for section variables - `:%`, `:/1024`, `:MaxValue`, `,decimals` - are parsed
// only on the branch that reads a measure's value, which a plugin call never reaches. So
// [NHM:GPU(vramused):/1048576] does not work and cannot be made to work, and AutoScale /
// NumOfDecimals on the meter act on a measure's value, not on text that was substituted
// before the meter ever saw it.
//
// Hence the fourth argument: [NHM:Storage(freespace,,1,auto)]. The arguments are the whole
// formatting surface a section variable can have.

// Rainmeter's scale table, suffixes and trailing-space behaviour, copied deliberately from
// Library/Measure.cpp GetScaledValue so that `auto` agrees digit-for-digit with AutoScale=1
// on a measure. The SPACE in front of the unit letter - and the lone trailing space when
// nothing needed scaling - is what makes Text=[NHM:Storage(freespace,,1,auto)]B read
// "80.1 GB" at 86 GB and "512 B" at 512 bytes. Dropping it would look tidier here and be
// wrong in every skin.
//
// Rainmeter's AutoScale=1k/2k (kilo as the lowest unit) is not offered: a skin that wants
// kilo always can divide by 1024 itself.
inline void NhmAutoScale(double& value, double base, const wchar_t*& suffix)
{
    static const wchar_t* const units[] = { L" T", L" G", L" M", L" k" };

    double step = base * base * base * base;
    suffix = L" ";

    for (int i = 0; i < 4; ++i, step /= base)
    {
        if (value >= step)
        {
            value /= step;
            suffix = units[i];
            return;
        }
    }
}

// `decimalsArg` and `scaleArg` are raw argv entries, so nullptr and L"" both mean "not
// given" - an omitted middle argument arrives as an empty string, which is why
// [NHM:CPU(usage,,0)] still means device 0.
//
// decimals: absent prints up to three decimals with trailing zeros trimmed, which is past
// any sensor's real precision and leaves whole numbers (and raw byte counts) whole.
//
// scale:    absent leaves the raw base unit. `auto` / `auto1024` / `auto1000` scale the way
//           AutoScale=1 / =2 would. Anything else is read as a divisor, with an optional
//           leading '/' so [NHM:CPU(clock,,0,/1000000)] reads the way Rainmeter's own
//           [Measure:/1000000] does. An unparsable divisor comes out 0 and is ignored, so a
//           typo prints the raw value rather than a lie.
//
// A NEGATIVE value is never scaled. -1 is the plugin's "no reading here" sentinel, and
// -1/1048576 or "-1 " would turn a legible sentinel into noise; a skin testing for -1 must
// keep working whatever formatting arguments it passes.
inline void FormatSectionValue(double value, const wchar_t* decimalsArg, const wchar_t* scaleArg,
                               std::wstring& out)
{
    int decimals = -1;
    if (decimalsArg && *decimalsArg)
    {
        decimals = _wtoi(decimalsArg);
        if (decimals < 0)  decimals = 0;
        if (decimals > 15) decimals = 15;
    }

    const wchar_t* suffix = L"";

    if (scaleArg && *scaleArg && value >= 0.0)
    {
        const wchar_t* p = scaleArg;
        while (*p == L' ')
            ++p;

        if (_wcsnicmp(p, L"auto", 4) == 0)
        {
            NhmAutoScale(value, wcscmp(p + 4, L"1000") == 0 ? 1000.0 : 1024.0, suffix);
        }
        else
        {
            if (*p == L'/')
                ++p;

            const double divisor = wcstod(p, nullptr);
            if (divisor > 0.0)
                value /= divisor;
        }
    }

    wchar_t buf[64];

    if (decimals >= 0)
    {
        swprintf_s(buf, L"%.*f", decimals, value);
        out = buf;
    }
    else
    {
        swprintf_s(buf, L"%.3f", value);
        out = buf;

        size_t last = out.find_last_not_of(L'0');
        if (last != std::wstring::npos && out[last] == L'.')
            --last; // nothing left after the point - drop it too

        out.erase(last + 1);
    }

    out += suffix;
}
