#pragma once

#include <memory>
#include <vector>

#include "Core/BaseBackend.h"
#include "Categories/GPU/IGpuProvider.h"
#include "Categories/GPU/NvidiaProvider.h"

class NvidiaBackend : public BaseBackend
{
public:
    double GetValue(uint32_t deviceIndex, uint32_t metricId) override;

protected:
    bool OnInitialize() override;
    void OnUpdate() override;

private:
    std::unique_ptr<IGpuProvider> m_provider;

    struct Snapshot
    {
        double utilization = 0.0;
        uint64_t memoryUsed = 0;
    };

    std::vector<Snapshot> m_snapshots;
};