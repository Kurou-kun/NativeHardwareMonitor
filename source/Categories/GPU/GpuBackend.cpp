#include "Categories/GPU/GpuBackend.h"
#include "Categories/GPU/NvidiaProvider.h"
#include "Types/GpuMetric.h"

bool GpuBackend::OnInitialize()
{
    // 1️⃣ Try NVIDIA
    {
        auto provider = std::make_unique<NvidiaProvider>();
        if (provider->Initialize() && provider->GetDeviceCount() > 0)
        {
            m_provider = std::move(provider);
        }
    }


    if (!m_provider)
        return false;

    m_snapshots.resize(m_provider->GetDeviceCount());
    return true;
}

void GpuBackend::OnUpdate()
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

double GpuBackend::GetValue(uint32_t deviceIndex, uint32_t metricId)
{
    if (!m_provider)
        return 0.0;

    if (deviceIndex >= m_snapshots.size())
        return 0.0;

    if (metricId == static_cast<uint32_t>(GpuMetric::UtilizationPercent))
        return m_snapshots[deviceIndex].utilization;

    if (metricId == static_cast<uint32_t>(GpuMetric::MemoryUsedBytes))
        return static_cast<double>(m_snapshots[deviceIndex].memoryUsed);

    return 0.0;
}