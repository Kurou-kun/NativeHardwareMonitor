#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <memory>
#include <string>
#include <vector>

enum class GpuVendor { Nvidia, AMD, Unknown };

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
    };

    std::vector<std::unique_ptr<IProvider>> m_providers;
    std::vector<DeviceEntry>                m_devices;
};
