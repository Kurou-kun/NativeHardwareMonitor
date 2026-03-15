#include "Categories/Storage/StorageBackend.h"
#include "Categories/Storage/WinApiStorageProvider.h"
#include "Utils/Debug.h"

bool StorageBackend::Initialize()
{

    if (m_initAttempted)
        return !m_initFailed;

    m_initAttempted = true;

    auto provider = std::make_unique<WinApiStorageProvider>();

    if (!provider->Initialize())
    {
        LOG_ERROR(L"[StorageBackend] WinApiStorageProvider initialization failed");
        m_initFailed = true;
        return false;
    }

    m_providers.push_back(std::move(provider));
    m_initialized = true;

    LOG_INFO(L"[StorageBackend] WinApiStorageProvider initialized");

    return true;
}

void StorageBackend::Update()
{
    if (!m_initialized)
        return;

    for (auto& provider : m_providers)
        provider->Update();
}

double StorageBackend::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    auto metric = static_cast<StorageMetric>(metricId);

    double value = 0.0;

    for (auto& provider : m_providers)
    {
        bool success = false;

        switch (metric)
        {
        case StorageMetric::ReadBytes:
            success = provider->GetReadBytes(deviceIndex, value);
            break;

        case StorageMetric::WriteBytes:
            success = provider->GetWriteBytes(deviceIndex, value);
            break;

        case StorageMetric::ReadSpeed:
            success = provider->GetReadSpeed(deviceIndex, value);
            break;

        case StorageMetric::WriteSpeed:
            success = provider->GetWriteSpeed(deviceIndex, value);
            break;

        case StorageMetric::UsedSpace:
            success = provider->GetUsedSpace(deviceIndex, value);
            break;

        case StorageMetric::FreeSpace:
            success = provider->GetFreeSpaceBytes(deviceIndex, value);
            break;

        case StorageMetric::TotalSpace:
            success = provider->GetTotalSpace(deviceIndex, value);
            break;

        default:
            return 0.0;
        }

        if (success)
            return value;
    }

    return 0.0;
}

uint32_t StorageBackend::GetDeviceCount() const
{
    uint32_t total = 0;

    for (const auto& provider : m_providers)
        total += provider->GetDeviceCount();

    return total;
}