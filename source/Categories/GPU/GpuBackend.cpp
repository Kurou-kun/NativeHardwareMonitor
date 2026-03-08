#include "Categories/GPU/GpuBackend.h"

#include "Categories/GPU/NvidiaProvider.h"
#include "Categories/GPU/AmdProvider.h"
#include "Utils/Debug.h"

bool GpuBackend::OnInitialize()
{

    // NVIDIA
    {
        auto provider = std::make_unique<NvidiaProvider>();

        if (provider->Initialize())
        {
            uint32_t count = provider->GetDeviceCount();

            for (uint32_t i = 0; i < count; ++i)
            {
                m_devices.push_back({ provider.get(), i, GpuVendor::Nvidia });
            }

            if (count > 0)
            {
                LOG_INFO(L"GPU: NVIDIA provider initialized (%u device(s))", count);
                m_providers.push_back(std::move(provider));
            }
        }
    }

    // AMD
    {
        auto provider = std::make_unique<AmdProvider>();

        if (provider->Initialize())
        {
            uint32_t count = provider->GetDeviceCount();

            for (uint32_t i = 0; i < count; ++i)
            {
                m_devices.push_back({ provider.get(), i, GpuVendor::AMD });
            }

            if (count > 0)
            {
                LOG_INFO(L"GPU: AMD provider initialized (%u device(s))", count);
                m_providers.push_back(std::move(provider));
            }
        }
    }


    if (m_devices.empty())
        return false;

    m_snapshots.resize(m_devices.size());

    return true;
}

void GpuBackend::OnUpdate()
{
    uint32_t count = static_cast<uint32_t>(m_devices.size());

    for (uint32_t i = 0; i < count; ++i)
    {
        auto& dev = m_devices[i];
        auto& snap = m_snapshots[i];

        double v;
        uint64_t memUsed = 0;
        uint64_t memTotal = 0;

        if (dev.provider->GetUsage(dev.index, v))
            snap.usage = v;

        if (dev.provider->GetVramUsed(dev.index, memUsed))
            snap.vramUsed = memUsed;

        if (dev.provider->GetVramTotal(dev.index, memTotal))
            snap.vramTotal = memTotal;

        if (dev.provider->GetTemperature(dev.index, v))
            snap.temperature = v;

        if (dev.provider->GetHotspotTemperature(dev.index, v))
            snap.hotspotTemperature = v;

        if (dev.provider->GetMemoryTemperature(dev.index, v))
            snap.memoryTemperature = v;

        if (dev.provider->GetCoreClock(dev.index, v))
            snap.coreClock = v;

        if (dev.provider->GetMemoryClock(dev.index, v))
            snap.memoryClock = v;

        if (dev.provider->GetFanSpeed(dev.index, v))
            snap.fanSpeed = v;

        if (dev.provider->GetFanSpeedRPM(dev.index, v))
            snap.fanSpeedRPM = v;

        if (dev.provider->GetPower(dev.index, v))
            snap.power = v;

        if (dev.provider->GetPowerLimit(dev.index, v))
            snap.powerLimit = v;
    }
}

double GpuBackend::GetValue(uint32_t metric, uint32_t index)
{
    if (index >= m_snapshots.size())
    {
        LOG_INFO(L"GPU index out of range");
        return 0.0;
    }

    const auto& snap = m_snapshots[index];

    switch (static_cast<GpuMetric>(metric))
    {
    case GpuMetric::Usage:
        return snap.usage;

    case GpuMetric::VramUsed:
        return static_cast<double>(snap.vramUsed);

    case GpuMetric::VramTotal:
        return static_cast<double>(snap.vramTotal);

    case GpuMetric::Temperature:
        return snap.temperature;

    case GpuMetric::HotspotTemperature:
        return snap.hotspotTemperature;

    case GpuMetric::MemoryTemperature:
        return snap.memoryTemperature;

    case GpuMetric::CoreClock:
        return snap.coreClock;

    case GpuMetric::MemoryClock:
        return snap.memoryClock;

    case GpuMetric::FanSpeed:
        return snap.fanSpeed;

    case GpuMetric::FanSpeedRPM:
        return snap.fanSpeedRPM;

    case GpuMetric::Power:
        return snap.power;

    case GpuMetric::PowerLimit:
        return snap.powerLimit;

    default:
        return 0.0;
    }
}