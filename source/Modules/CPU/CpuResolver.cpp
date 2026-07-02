#include "Modules/CPU/CpuResolver.h"
#include "Providers/WinApi/WinApiCpuProvider.h"
#include "Utils/Debug.h"

bool CpuResolver::Initialize()
{
    auto provider = std::make_unique<WinApiCpuProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"CpuResolver: WinApiCpuProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"CpuResolver: WinApiCpuProvider initialized (%u device(s), incl. total)", provider->GetDeviceCount());
    m_provider = std::move(provider);

    return true;
}

uint32_t CpuResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void CpuResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool CpuResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}
