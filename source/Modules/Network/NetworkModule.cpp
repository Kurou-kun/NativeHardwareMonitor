#include "Modules/Network/NetworkModule.h"
#include "Modules/Network/NetworkResolver.h"

bool NetworkModule::Initialize()
{
    if (m_initialized)
        return true;

    m_resolver = std::make_unique<NetworkResolver>();

    if (!m_resolver->Initialize())
        return false;

    m_snapshots.resize(m_resolver->GetDeviceCount());
    m_initialized = true;

    return true;
}

void NetworkModule::GatherDevices(const std::vector<uint32_t>& devices)
{
    if (!m_initialized)
        return;

    if (devices.empty())
    {
        GatherAll();
        return;
    }

    uint32_t count = m_resolver->GetDeviceCount();
    for (uint32_t i : devices)
    {
        if (i < count)
            m_resolver->GatherSnapshot(i, m_snapshots[i]);
    }
}

void NetworkModule::GatherAll()
{
    if (!m_initialized)
        return;

    uint32_t count = m_resolver->GetDeviceCount();

    for (uint32_t i = 0; i < count; ++i)
        m_resolver->GatherSnapshot(i, m_snapshots[i]);
}

double NetworkModule::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    if (deviceIndex >= m_snapshots.size())
        return -2.0;

    return m_snapshots[deviceIndex].Get(metricId);
}

bool NetworkModule::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_resolver ? m_resolver->GetString(metricId, deviceIndex, out) : false;
}

uint32_t NetworkModule::GetDeviceCount() const
{
    return m_resolver ? m_resolver->GetDeviceCount() : 0;
}
