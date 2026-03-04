#include "Categories/GPU/NvidiaBackend.h"
#include "Types/GpuMetric.h"

bool NvidiaBackend::OnInitialize()
{
    m_provider = std::make_unique<NvidiaProvider>();

    if (!m_provider->Initialize())
        return false;

    uint32_t count = m_provider->GetDeviceCount();
    m_snapshots.resize(count);

    return true;
}

void NvidiaBackend::OnUpdate()
{
    uint32_t count = m_provider->GetDeviceCount();

    for (uint32_t i = 0; i < count; ++i)
    {
        double util = 0.0;
        uint64_t mem = 0;

        if (m_provider->GetUtilization(i, util))
            m_snapshots[i].utilization = util;

        if (m_provider->GetMemoryUsed(i, mem))
            m_snapshots[i].memoryUsed = mem;
    }
}

double NvidiaBackend::GetValue(uint32_t deviceIndex, uint32_t metricId)
{
    if (deviceIndex >= m_snapshots.size())
        return 0.0;

    if (metricId == static_cast<uint32_t>(GpuMetric::UtilizationPercent))
        return m_snapshots[deviceIndex].utilization;

    if (metricId == static_cast<uint32_t>(GpuMetric::MemoryUsedBytes))
        return static_cast<double>(m_snapshots[deviceIndex].memoryUsed);

    return 0.0;
}