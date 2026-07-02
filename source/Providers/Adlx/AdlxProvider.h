#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <vector>
#include <ISystem.h>
#include <IPerformanceMonitoring.h>

class AdlxProvider : public IProvider
{
public:
    ~AdlxProvider()
    {
        for (auto* gpu : m_gpus) if (gpu) gpu->Release();
        if (m_perf) m_perf->Release();
    }

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    adlx::IADLXSystem*                      m_system = nullptr;
    adlx::IADLXPerformanceMonitoringServices* m_perf  = nullptr;
    std::vector<adlx::IADLXGPU*>            m_gpus;
};
