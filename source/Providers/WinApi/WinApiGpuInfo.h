#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Last-resort Windows-side source (DXGI + registry) for GPU Name/DriverVersion,
// used only when the vendor SDK can't supply them — currently just the
// ADL2-only fallback path (no AMD hardware here to verify against).
class WinApiGpuInfo
{
public:
    bool GetName(uint32_t pciVendorId, uint32_t ordinal, std::wstring& out);
    bool GetDriverVersion(uint32_t pciVendorId, uint32_t ordinal, std::wstring& out);

private:
    struct AdapterEntry
    {
        std::wstring name;
        uint32_t     vendorId = 0;
    };

    std::vector<AdapterEntry> m_adapters;
    bool                      m_enumerated = false;

    bool EnsureEnumerated();
};
