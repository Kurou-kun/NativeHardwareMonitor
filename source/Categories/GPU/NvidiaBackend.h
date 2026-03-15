#pragma once

#include "Core/BaseBackend.h"
#include "Categories/GPU/NvidiaProvider.h"

#include <vector>
#include <memory>
#include <stdint.h>

class NvidiaBackend : public BaseBackend
{
public:

    struct Snapshot
    {
        double utilization = 0.0;
        uint64_t memoryUsed = 0;
    };

protected:

    bool OnInitialize() override;
    void OnUpdate() override;

public:

    double GetValue(uint32_t deviceIndex, uint32_t metricId) override;

private:

    std::unique_ptr<NvidiaProvider> m_provider;

    std::vector<Snapshot> m_snapshots;
};