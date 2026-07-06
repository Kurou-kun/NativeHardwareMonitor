#include "Modules/Display/DisplayResolver.h"
#include "Providers/WinApi/WinApiDisplayProvider.h"
#include "Utils/Debug.h"

bool DisplayResolver::Initialize()
{
    auto provider = std::make_unique<WinApiDisplayProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"DisplayResolver: WinApiDisplayProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"DisplayResolver: WinApiDisplayProvider initialized");
    m_provider = std::move(provider);

    return true;
}

uint32_t DisplayResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void DisplayResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool DisplayResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}
