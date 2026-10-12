#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"
#include "Providers/WinApi/WinApiGpuInfo.h"
#include "Providers/WinApi/WinApiGpuProvider.h"

#include <memory>
#include <string>
#include <vector>

enum class GpuVendor { Nvidia, AMD, Intel, Unknown };

// An IProvider that resolves between providers: NVML/NVAPI for NVIDIA, ADLX/ADL2 for AMD,
// IGCL for Intel, Windows' own counters behind all of them, and a DXGI/registry last resort
// for names. Being an IProvider is what lets the shared Module hold it exactly as it holds a
// single-source provider - GPU is the only category that needs this layer.
class GpuResolver : public IProvider
{
public:
    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    struct DeviceEntry
    {
        IProvider* provider         = nullptr;
        uint32_t   localIndex       = 0;
        GpuVendor  vendor           = GpuVendor::Unknown;

        // Secondary provider queried only for metrics the primary can't supply
        IProvider* backupProvider   = nullptr;
        uint32_t   backupLocalIndex = 0;

        // Windows' own counters, queried after both of the above and only for what they
        // still left unset — usage and VRAM on hardware whose SDK answers neither.
        IProvider* winProvider      = nullptr;
        uint32_t   winLocalIndex    = 0;
    };

    void AttachWindowsCounters();

    std::vector<std::unique_ptr<IProvider>> m_providers;
    std::vector<DeviceEntry>                m_devices;
    WinApiGpuInfo                           m_winFallback;
};
