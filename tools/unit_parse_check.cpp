// Pins the two pieces of logic that the 2026-10-17 code revision (#13) found wrong,
// both of which are pure functions and therefore checkable without any hardware:
//
//   1. ParsePciIdFromPnp  - ADL's strPNPString is produced by Windows and the driver,
//                           so its case is not ours to assume. A case-sensitive search
//                           returned an empty PciDeviceId on a lower-case string, and
//                           the extracted digits were not normalised, which quietly
//                           broke the "same format as the NVIDIA path" contract.
//
//   2. IgclFrequencyHz    - IGCL declares the units of every telemetry item. The
//                           frequency items are MHz while every other provider
//                           publishes Hz, and the raw value was passed straight
//                           through: an Intel core clock read 2100 rather than 2.1e9.
//
// Both are the same family as #12: an assumption about something an external API
// produced, never verified, on a path the development machine does not exercise.
//
// Build and run (no framework, no project entry):
//
//   cl /EHsc /std:c++17 /I source /I sdk/igcl tools\unit_parse_check.cpp

#include "Providers/Adl2/Adl2Parse.h"
#include "Providers/Igcl/IgclUnits.h"

#include <cstdio>
#include <string>

static int failures = 0;

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) {                                                     \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #expr);    \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static void PnpChecks()
{
    const size_t MAX = 256;

    // The spelling Windows actually uses today.
    CHECK(ParsePciIdFromPnp("PCI\\VEN_1002&DEV_73FF&SUBSYS_23021462&REV_C1", MAX) == L"1002:73FF");

    // The regression this check exists for: lower case must parse, and must come back
    // upper case so it still matches what NVML, NVAPI and IGCL format with %04X.
    CHECK(ParsePciIdFromPnp("pci\\ven_1002&dev_73ff&subsys_23021462", MAX) == L"1002:73FF");
    CHECK(ParsePciIdFromPnp("PCI\\Ven_8086&Dev_56A0", MAX) == L"8086:56A0");

    // A real Raphael iGPU string, and a real NVIDIA one.
    CHECK(ParsePciIdFromPnp("PCI\\VEN_1002&DEV_13C0&SUBSYS_7E571462&REV_C5", MAX) == L"1002:13C0");
    CHECK(ParsePciIdFromPnp("PCI\\VEN_10DE&DEV_2882&SUBSYS_410B1458&REV_A1", MAX) == L"10DE:2882");

    // Nothing to find, or truncated before the digits: empty, never a partial id.
    CHECK(ParsePciIdFromPnp("", MAX).empty());
    CHECK(ParsePciIdFromPnp(nullptr, MAX).empty());
    CHECK(ParsePciIdFromPnp("ROOT\\BASICDISPLAY\\0000", MAX).empty());
    CHECK(ParsePciIdFromPnp("PCI\\VEN_10", MAX).empty());
    CHECK(ParsePciIdFromPnp("PCI\\VEN_1002&DEV_73", MAX).empty());

    // maxLen is honoured, so a string without a terminator cannot run away.
    CHECK(ParsePciIdFromPnp("PCI\\VEN_1002&DEV_73FF", 10).empty());
}

static void FrequencyChecks()
{
    double hz = 0.0;

    // The conversion the Intel path was missing entirely.
    CHECK(IgclFrequencyHz(CTL_UNITS_FREQUENCY_MHZ, 2100.0, hz) && hz == 2100000000.0);
    CHECK(IgclFrequencyHz(CTL_UNITS_FREQUENCY_MHZ, 0.0, hz) && hz == 0.0);

    // Transfer-rate units some memory subsystems report instead of a clock.
    CHECK(IgclFrequencyHz(CTL_UNITS_OPERATIONS_MTS, 14000.0, hz) && hz == 14000000000.0);
    CHECK(IgclFrequencyHz(CTL_UNITS_OPERATIONS_GTS, 14.0, hz) && hz == 14000000000.0);

    // Anything else must be refused rather than published in unknown units - a wrong
    // number looks right, where -1 is documented to mean "no reading".
    hz = -12345.0;
    CHECK(!IgclFrequencyHz(CTL_UNITS_UNKNOWN, 2100.0, hz));
    CHECK(!IgclFrequencyHz(CTL_UNITS_POWER_WATTS, 2100.0, hz));
    CHECK(!IgclFrequencyHz(CTL_UNITS_PERCENT, 2100.0, hz));
    CHECK(hz == -12345.0);   // and must not have been written to

    // Energy and time guard the ΔJoules/Δs power calculation.
    double j = 0.0, s = 0.0;
    CHECK(IgclEnergyJoules(CTL_UNITS_ENERGY_JOULES, 1234.5, j) && j == 1234.5);
    CHECK(!IgclEnergyJoules(CTL_UNITS_POWER_MILLIWATTS, 1234.5, j));
    CHECK(IgclTimeSeconds(CTL_UNITS_TIME_SECONDS, 42.0, s) && s == 42.0);
    CHECK(!IgclTimeSeconds(CTL_UNITS_UNKNOWN, 42.0, s));
}

int main()
{
    PnpChecks();
    FrequencyChecks();

    if (failures)
    {
        std::printf("unit_parse_check: %d FAILURES\n", failures);
        return 1;
    }

    std::printf("unit_parse_check: all assertions passed\n");
    return 0;
}
