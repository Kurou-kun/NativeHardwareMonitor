#include "Modules/Memory/MemoryResolver.h"
#include "Providers/WinApi/WinApiMemoryProvider.h"
#include "Utils/Debug.h"

bool MemoryResolver::Initialize()
{
    auto provider = std::make_unique<WinApiMemoryProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"MemoryResolver: WinApiMemoryProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"MemoryResolver: WinApiMemoryProvider initialized");
    m_provider = std::move(provider);

    return true;
}

uint32_t MemoryResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void MemoryResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool MemoryResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}
