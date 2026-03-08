#pragma once

#include "Core/BaseBackend.h"
#include "Categories/GPU/IGpuProvider.h"
#include "Types/GpuMetric.h"

#include <vector>
#include <memory>

enum class GpuVendor
{
    Nvidia,
    AMD,
    Intel,
    Unknown
};

class GpuBackend : public BaseBackend
{
public:

    struct Snapshot
    {
        double usage = 0.0;

        uint64_t vramUsed = 0;
        uint64_t vramTotal = 0;

        double temperature = 0.0;
        double hotspotTemperature = 0.0;
        double memoryTemperature = 0.0;

        double coreClock = 0.0;
        double memoryClock = 0.0;

        double fanSpeed = 0.0;
        double fanSpeedRPM = 0.0;

        double power = 0.0;
        double powerLimit = 0.0;
    };

protected:

    bool OnInitialize() override;
    void OnUpdate() override;

public:

    double GetValue(uint32_t metric, uint32_t index) override;

private:

    struct Device
    {
        IGpuProvider* provider = nullptr;
        uint32_t index = 0;
        GpuVendor vendor = GpuVendor::Unknown;
    };

    std::vector<std::unique_ptr<IGpuProvider>> m_providers;
    std::vector<Device> m_devices;

    std::vector<Snapshot> m_snapshots;
};