#include "Modules/Ping/PingResolver.h"
#include "Providers/WinApi/WinApiPingProvider.h"
#include "Utils/Debug.h"

bool PingResolver::Initialize()
{
    auto provider = std::make_unique<WinApiPingProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"PingResolver: WinApiPingProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"PingResolver: WinApiPingProvider initialized");
    m_provider = std::move(provider);

    return true;
}

uint32_t PingResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void PingResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool PingResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}

uint32_t PingResolver::ResolveTarget(const std::wstring& host, uint32_t intervalMs)
{
    return m_provider ? m_provider->ResolveTarget(host, intervalMs) : 0;
}
