#pragma once

#include <string>
#include <cwctype>

// Pulling "1002:73FF" out of ADL's strPNPString, kept here rather than inline in the
// provider so tools/pnp_parse_check.cpp can pin it without dragging in the ADL SDK.
//
// The case handling is the point. A PNP device string is produced by Windows and the
// driver, not by us, and its case is not promised - WinApiGpuInfo::GetDriverVersion
// already upper-cases MatchingDeviceID before matching it for exactly this reason. A
// case-sensitive search here would return an empty PciDeviceId on any driver that
// spelled the string in lower case, which is the same silent-nothing failure the
// Windows GPU counters had in #12.
//
// Normalising the extracted digits matters too: NVML, NVAPI and IGCL all format this
// metric with %04X, and the whole reason ADL2 reports "vendor:device" is so a skin can
// compare the two without special cases. Copying the driver's spelling through would
// quietly break that for a lower-case string.
inline std::wstring ParsePciIdFromPnp(const char* pnp, size_t maxLen)
{
    if (!pnp)
        return {};

    std::string s(pnp, strnlen_s(pnp, maxLen));
    for (char& c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    const size_t ven = s.find("VEN_");
    const size_t dev = s.find("DEV_");
    if (ven == std::string::npos || dev == std::string::npos ||
        ven + 8 > s.size() || dev + 8 > s.size())
        return {};

    const std::string id = s.substr(ven + 4, 4) + ":" + s.substr(dev + 4, 4);
    return std::wstring(id.begin(), id.end());
}
