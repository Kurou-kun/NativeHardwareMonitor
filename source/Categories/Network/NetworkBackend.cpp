#include "Categories/Network/NetworkBackend.h"
#include "Categories/Network/WinApiNetworkProvider.h"

#include "Types/NetworkMetric.h"
#include "Utils/Debug.h"

bool NetworkBackend::Initialize()
{
    if (m_initAttempted)
        return !m_initFailed;

    m_initAttempted = true;

    auto provider = std::make_unique<WinApiNetworkProvider>();

    if (!provider->Initialize())
    {
        LOG_ERROR(L"Network provider initialization failed");
        m_initFailed = true;
        return false;
    }

    LOG_INFO(L"Network provider initialized");

    m_providers.push_back(std::move(provider));

    return true;
}

void NetworkBackend::Update()
{
    for (auto& provider : m_providers)
    {
        provider->Update();
    }
}

double NetworkBackend::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    if (m_providers.empty())
        return 0.0;

    auto metric = static_cast<NetworkMetric>(metricId);
    auto& provider = m_providers[0];

    double value = 0.0;

    switch (metric)
    {
    case NetworkMetric::Download:
        if (provider->GetDownload(deviceIndex, value)) return value;
        break;

    case NetworkMetric::Upload:
        if (provider->GetUpload(deviceIndex, value)) return value;
        break;

    case NetworkMetric::DownloadTotal:
        if (provider->GetDownloadTotal(deviceIndex, value)) return value;
        break;

    case NetworkMetric::UploadTotal:
        if (provider->GetUploadTotal(deviceIndex, value)) return value;
        break;

    case NetworkMetric::Speed:
        if (provider->GetSpeed(deviceIndex, value)) return value;
        break;

    default:
        break;
    }

    return 0.0;
}

uint32_t NetworkBackend::GetDeviceCount() const
{
    if (m_providers.empty())
        return 0;

    return m_providers[0]->GetDeviceCount();
}