#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"
#include "Providers/WinApi/WinApiGpuInfo.h"
#include "Providers/WinApi/WinApiGpuProvider.h"

#include <memory>
#include <string>
#include <vector>

enum class GpuVendor { Nvidia, AMD, Intel, Unknown };

class GpuResolver
{
public:
    bool     Initialize();
    uint32_t GetDeviceCount() const;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap);
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out);

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
