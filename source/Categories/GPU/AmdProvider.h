#pragma once

#include "Categories/GPU/IGpuProvider.h"

#include <vector>

#include <ISystem.h>
#include <IPerformanceMonitoring.h>

class AmdProvider : public IGpuProvider
{
public:

    bool Initialize() override;
    uint32_t GetDeviceCount() const override;

    bool GetUsage(uint32_t index, double& value) override;

    bool GetTemperature(uint32_t index, double& value) override;
    bool GetHotspotTemperature(uint32_t index, double& value) override;

    bool GetCoreClock(uint32_t index, double& value) override;

    bool GetPower(uint32_t index, double& value) override;

    bool GetMemoryClock(uint32_t index, double& value);
    bool GetFanSpeed(uint32_t index, double& value);

private:

    adlx::IADLXSystem* m_system = nullptr;
    adlx::IADLXPerformanceMonitoringServices* m_perf = nullptr;

    std::vector<adlx::IADLXGPU*> m_gpus;

    adlx::IADLXGPUMetrics* GetMetrics(uint32_t index);
};