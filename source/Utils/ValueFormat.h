#pragma once

#include <string>

#include <cstdio>
#include <cwchar>

// Printing a section variable happens HERE, in the plugin, and that is not a design
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
// What this plugin does about that is: NOTHING BEYOND A BARE NUMBER, deliberately. A
// section variable prints the value in its raw base unit - bytes, hertz, bytes per second
// - because a bare number is the one form a skin can compute with. Scaling it here was
// tried and removed: it would have made the plugin own unit and locale decisions that
// belong to the skin, and it bought nothing a skin cannot do in one line itself.
//
//     Lua:  local gb = tonumber(SKIN:ReplaceVariables('[NHM:GPU("vramused",,15)]')) / 1073741824
//     Calc: [MeasureVram] Measure=Calc / Formula=[NHM:GPU(vramused)] / DynamicVariables=1
//
// The Calc form is worth knowing twice over: it hands the number back to Rainmeter as a
// measure value, so AutoScale, NumOfDecimals, Percentual and bar ranges all work again.

// `decimalsArg` is a raw argv entry, so nullptr and L"" both mean "not given" - an omitted
// middle argument arrives as an empty string, which is why [NHM:CPU(usage,,0)] still means
// device 0.
//
// Absent prints up to three decimals with trailing zeros trimmed: past any sensor's real
// precision, and it leaves whole numbers (and raw byte counts) whole rather than dressing
// them up. Pass ,,15 when the value feeds a formula or Lua and every digit matters.
//
// Sentinels pass through untouched by construction - -1 and -2 are not measurements, and a
// skin testing for them has to keep working whatever arguments it passes.
inline void FormatSectionValue(double value, const wchar_t* decimalsArg, std::wstring& out)
{
    int decimals = -1;
    if (decimalsArg && *decimalsArg)
    {
        decimals = _wtoi(decimalsArg);
        if (decimals < 0)  decimals = 0;
        if (decimals > 15) decimals = 15;
    }

    wchar_t buf[64];

    if (decimals >= 0)
    {
        swprintf_s(buf, L"%.*f", decimals, value);
        out = buf;
        return;
    }

    swprintf_s(buf, L"%.3f", value);
    out = buf;

    size_t last = out.find_last_not_of(L'0');
    if (last != std::wstring::npos && out[last] == L'.')
        --last; // nothing left after the point - drop it too

    out.erase(last + 1);
}
