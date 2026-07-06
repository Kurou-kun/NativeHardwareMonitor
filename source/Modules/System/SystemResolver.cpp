#include "Modules/System/SystemResolver.h"
#include "Providers/WinApi/WinApiSystemProvider.h"
#include "Utils/Debug.h"

bool SystemResolver::Initialize()
{
    auto provider = std::make_unique<WinApiSystemProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"SystemResolver: WinApiSystemProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"SystemResolver: WinApiSystemProvider initialized");
    m_provider = std::move(provider);

    return true;
}

uint32_t SystemResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void SystemResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool SystemResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}
