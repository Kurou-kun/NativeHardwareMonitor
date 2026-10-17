#pragma once

#include "igcl_api.h"

// IGCL is the only vendor API that TELLS you what units a reading is in:
// ctl_oc_telemetry_item_t carries a ctl_units_t beside the value. Everything here
// exists because the first implementation read that field for voltage and fan speed
// and ignored it everywhere else, publishing a frequency in whatever the driver
// happened to report.
//
// That mattered: every other provider publishes clocks in Hz (NVML and ADLX multiply
// MHz by 1e6, NVAPI multiplies kHz by 1e3, ADL2's Overdrive5 path multiplies its
// 10 kHz units by 1e4), while this one passed the raw number straight through. On
// Intel hardware CoreClock would have read 2100 instead of 2.1e9 - a factor of a
// million, on the one provider that has never run on the silicon it targets (#2).
//
// The rule followed below: convert what the item SAYS it is, and publish nothing when
// it says something unexpected. Publishing a number in unknown units is worse than a
// -1, because -1 is documented to mean "no reading" while a wrong number looks real.

// Frequency item -> Hz. False when the declared units are not a frequency this
// understands, so the caller can skip the metric instead of guessing.
inline bool IgclFrequencyHz(ctl_units_t units, double raw, double& hz)
{
    switch (units)
    {
    case CTL_UNITS_FREQUENCY_MHZ:
        hz = raw * 1000000.0;
        return true;

    // Transfer rates, reported by some memory subsystems in place of a clock. They
    // are not the same quantity as a clock - GDDR runs several transfers per cycle -
    // but they are the only frequency-shaped reading on offer, and converting to Hz
    // keeps the unit honest even where the semantics differ by a memory generation.
    case CTL_UNITS_OPERATIONS_MTS:
        hz = raw * 1000000.0;
        return true;
    case CTL_UNITS_OPERATIONS_GTS:
        hz = raw * 1000000000.0;
        return true;

    default:
        return false;
    }
}

// Energy item -> Joules, for the ΔJoules/Δs power calculation. The counter is
// documented as Joules and in practice reports them, but the item declares its units
// and a millijoule counter would read a thousand times high with nothing to show it
// was wrong.
inline bool IgclEnergyJoules(ctl_units_t units, double raw, double& joules)
{
    if (units != CTL_UNITS_ENERGY_JOULES)
        return false;

    joules = raw;
    return true;
}

// Timestamp item -> seconds, the denominator of the same calculation.
inline bool IgclTimeSeconds(ctl_units_t units, double raw, double& seconds)
{
    if (units != CTL_UNITS_TIME_SECONDS)
        return false;

    seconds = raw;
    return true;
}
