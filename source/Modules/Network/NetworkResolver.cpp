#include "Modules/Network/NetworkResolver.h"
#include "Providers/WinApi/WinApiNetworkProvider.h"
#include "Utils/Debug.h"

bool NetworkResolver::Initialize()
{
    auto provider = std::make_unique<WinApiNetworkProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"NetworkResolver: WinApiNetworkProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"NetworkResolver: WinApiNetworkProvider initialized (%u adapter(s))", provider->GetDeviceCount());
    m_provider = std::move(provider);

    return true;
}

uint32_t NetworkResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void NetworkResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool NetworkResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}
