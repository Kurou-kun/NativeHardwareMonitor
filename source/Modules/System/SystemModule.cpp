#include "Modules/System/SystemModule.h"
#include "Modules/System/SystemResolver.h"

bool SystemModule::Initialize()
{
    if (m_initialized)
        return true;

    m_resolver = std::make_unique<SystemResolver>();

    if (!m_resolver->Initialize())
        return false;

    m_snapshots.resize(m_resolver->GetDeviceCount());
    m_initialized = true;

    return true;
}

void SystemModule::GatherAll()
{
    if (!m_initialized)
        return;

    uint32_t count = m_resolver->GetDeviceCount();

    for (uint32_t i = 0; i < count; ++i)
        m_resolver->GatherSnapshot(i, m_snapshots[i]);
}

double SystemModule::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    if (deviceIndex >= m_snapshots.size())
        return -2.0;

    return m_snapshots[deviceIndex].Get(metricId);
}

bool SystemModule::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_resolver ? m_resolver->GetString(metricId, deviceIndex, out) : false;
}

uint32_t SystemModule::GetDeviceCount() const
{
    return m_resolver ? m_resolver->GetDeviceCount() : 0;
}
