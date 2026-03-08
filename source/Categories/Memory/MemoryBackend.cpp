#include "Categories/Memory/MemoryBackend.h"
#include "Categories/Memory/WinApiMemoryProvider.h"

#include "Types/MemoryMetric.h"
#include "Utils/Debug.h"

bool MemoryBackend::Initialize()
{
    if (m_initAttempted)
        return !m_initFailed;

    m_initAttempted = true;

    auto provider = std::make_unique<WinApiMemoryProvider>();

    if (!provider->Initialize())
    {
        LOG_ERROR(L"Memory provider initialization failed");
        m_initFailed = true;
        return false;
    }

    LOG_INFO(L"Memory provider initialized");

    m_providers.push_back(std::move(provider));

    return true;
}

void MemoryBackend::Update()
{
    for (auto& provider : m_providers)
    {
        provider->Update();
    }
}

double MemoryBackend::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    if (m_providers.empty())
        return 0.0;

    auto metric = static_cast<MemoryMetric>(metricId);
    auto& provider = m_providers[0];

    double value = 0.0;

    switch (metric)
    {
    case MemoryMetric::Used:
        if (provider->GetUsed(deviceIndex, value)) return value;
        break;

    case MemoryMetric::Free:
        if (provider->GetFree(deviceIndex, value)) return value;
        break;

    case MemoryMetric::Total:
        if (provider->GetTotal(deviceIndex, value)) return value;
        break;

    case MemoryMetric::UsedPercent:
        if (provider->GetUsedPercent(deviceIndex, value)) return value;
        break;

    default:
        break;
    }

    return 0.0;
}

uint32_t MemoryBackend::GetDeviceCount() const
{
    if (m_providers.empty())
        return 0;

    return m_providers[0]->GetDeviceCount();
}