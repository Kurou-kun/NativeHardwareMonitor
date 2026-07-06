#include "Modules/Motherboard/MotherboardResolver.h"
#include "Providers/WinApi/WinApiMotherboardProvider.h"
#include "Utils/Debug.h"

bool MotherboardResolver::Initialize()
{
    auto provider = std::make_unique<WinApiMotherboardProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"MotherboardResolver: WinApiMotherboardProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"MotherboardResolver: WinApiMotherboardProvider initialized");
    m_provider = std::move(provider);

    return true;
}

uint32_t MotherboardResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void MotherboardResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool MotherboardResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}
