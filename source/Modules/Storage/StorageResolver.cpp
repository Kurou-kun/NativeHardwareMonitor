#include "Modules/Storage/StorageResolver.h"
#include "Providers/WinApi/WinApiStorageProvider.h"
#include "Utils/Debug.h"

bool StorageResolver::Initialize()
{
    auto provider = std::make_unique<WinApiStorageProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"StorageResolver: WinApiStorageProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"StorageResolver: WinApiStorageProvider initialized (%u drive(s))", provider->GetDeviceCount());
    m_provider = std::move(provider);

    return true;
}

uint32_t StorageResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void StorageResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool StorageResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}
