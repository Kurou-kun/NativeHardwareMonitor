#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <pdh.h>
#include <string>
#include <vector>

// Vendor-neutral GPU source: the same counters Task Manager's GPU tab reads, plus DXGI
// for the memory totals. It knows nothing about any vendor SDK, so it answers on an AMD
// iGPU with no ADLX, an Intel GPU with no ControlLib, and any card whose SDK is missing
// or too old — which is exactly where the vendor providers leave holes.
//
// It only ever supplies Usage, VramUsed and VramTotal. Temperature, power, voltage and
// fan have no Windows-side source at all without a kernel driver, which is the thing
// this plugin exists not to need.
class WinApiGpuProvider : public IProvider
{
public:
    ~WinApiGpuProvider() { Shutdown(); }

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

    // Which local device is a given vendor's Nth GPU — how the resolver pairs one of
    // these adapters with a device a vendor SDK already enumerated. -1 when absent.
    int FindAdapter(uint32_t pciVendorId, uint32_t ordinal) const;

private:
    struct Adapter
    {
        std::wstring luidKey;        // "luid_0x%08x_0x%08x", as PDH spells it in instance names
        std::wstring description;
        uint32_t     vendorId       = 0;
        double       dedicatedTotal = 0.0;
        double       sharedTotal    = 0.0;
    };

    void Shutdown();
    bool EnumerateAdapters();

    std::vector<Adapter> m_adapters;

    PDH_HQUERY   m_query     = nullptr;
    PDH_HCOUNTER m_engine    = nullptr;
    PDH_HCOUNTER m_dedicated = nullptr;
    PDH_HCOUNTER m_shared    = nullptr;
};
