#include "Modules/Ping/PingModule.h"
#include "Modules/Ping/PingResolver.h"

bool PingModule::Initialize()
{
    if (m_initialized)
        return true;

    m_resolver = std::make_unique<PingResolver>();

    if (!m_resolver->Initialize())
        return false;

    // No m_snapshots.resize() here, unlike other modules — Ping starts with
    // zero targets; ResolveTarget() grows m_snapshots as hosts are registered.
    m_initialized = true;
    return true;
}

void PingModule::GatherAll()
{
    if (!m_initialized)
        return;

    uint32_t count = m_resolver->GetDeviceCount();
    if (m_snapshots.size() < count)
        m_snapshots.resize(count);

    for (uint32_t i = 0; i < count; ++i)
        m_resolver->GatherSnapshot(i, m_snapshots[i]);
}

double PingModule::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    if (deviceIndex >= m_snapshots.size())
        return -2.0;

    return m_snapshots[deviceIndex].Get(metricId);
}

bool PingModule::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_resolver ? m_resolver->GetString(metricId, deviceIndex, out) : false;
}

uint32_t PingModule::GetDeviceCount() const
{
    return m_resolver ? m_resolver->GetDeviceCount() : 0;
}

uint32_t PingModule::ResolveTarget(const std::wstring& host, uint32_t intervalMs)
{
    if (!m_resolver)
        return 0;

    uint32_t index = m_resolver->ResolveTarget(host, intervalMs);

    if (m_snapshots.size() <= index)
        m_snapshots.resize(index + 1);

    return index;
}
