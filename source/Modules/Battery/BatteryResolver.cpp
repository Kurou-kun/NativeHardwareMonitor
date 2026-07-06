#include "Modules/Battery/BatteryResolver.h"
#include "Providers/WinApi/WinApiBatteryProvider.h"
#include "Utils/Debug.h"

bool BatteryResolver::Initialize()
{
    auto provider = std::make_unique<WinApiBatteryProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"BatteryResolver: WinApiBatteryProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"BatteryResolver: WinApiBatteryProvider initialized");
    m_provider = std::move(provider);

    return true;
}

uint32_t BatteryResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void BatteryResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool BatteryResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}
