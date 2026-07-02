#include "Modules/CPU/CpuModule.h"
#include "Modules/CPU/CpuResolver.h"

bool CpuModule::Initialize()
{
    if (m_initialized)
        return true;

    m_resolver = std::make_unique<CpuResolver>();

    if (!m_resolver->Initialize())
        return false;

    m_snapshots.resize(m_resolver->GetDeviceCount());
    m_initialized = true;

    return true;
}

void CpuModule::GatherAll()
{
    if (!m_initialized)
        return;

    uint32_t count = m_resolver->GetDeviceCount();

    for (uint32_t i = 0; i < count; ++i)
        m_resolver->GatherSnapshot(i, m_snapshots[i]);
}

double CpuModule::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    if (deviceIndex >= m_snapshots.size())
        return 0.0;

    return m_snapshots[deviceIndex].Get(metricId);
}

bool CpuModule::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (!m_resolver)
        return false;

    return m_resolver->GetString(metricId, deviceIndex, out);
}

uint32_t CpuModule::GetDeviceCount() const
{
    if (!m_resolver)
        return 0;

    return m_resolver->GetDeviceCount();
}
