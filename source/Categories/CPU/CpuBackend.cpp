#include "Categories/CPU/CpuBackend.h"

#include "Categories/CPU/WinApiProvider.h"

#include "Types/CpuMetric.h"
#include "Utils/Debug.h"

#include <intrin.h>
#include <cstring>

bool CpuBackend::Initialize()
{
    if (m_initAttempted)
        return !m_initFailed;

    m_initAttempted = true;

    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);

    char vendor[13];
    memcpy(vendor + 0, &cpuInfo[1], 4);
    memcpy(vendor + 4, &cpuInfo[3], 4);
    memcpy(vendor + 8, &cpuInfo[2], 4);
    vendor[12] = '\0';

    std::unique_ptr<ICpuProvider> provider;

    if (strcmp(vendor, "GenuineIntel") == 0)
    {
        provider = std::make_unique<WinApiProvider>();
    }
    else if (strcmp(vendor, "AuthenticAMD") == 0)
    {
        provider = std::make_unique<WinApiProvider>();
    }
    else
    {
        m_initFailed = true;
        return false;
    }

    if (!provider->Initialize())
    {
        m_initFailed = true;
        return false;
    }

    m_providers.push_back(std::move(provider));

    m_initialized = true;

    return true;
}

void CpuBackend::Update()
{
    for (auto& provider : m_providers)
    {
        provider->Update();
    }
}

double CpuBackend::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    auto metric = static_cast<CpuMetric>(metricId);

    if (m_providers.empty())
        return 0.0;

    auto& provider = m_providers[0]; // CPU currently has one provider

    double value = 0.0;

    switch (metric)
    {
    case CpuMetric::Usage:
    {
        if (deviceIndex == 0)
            provider->GetTotalUsage(value);
        else
            provider->GetCoreUsage(deviceIndex - 1, value);

        return value;
    }

    case CpuMetric::Clock:
    {
        provider->GetClock(value);
        return value;
    }
    }

    return 0.0;
}

uint32_t CpuBackend::GetDeviceCount() const
{
    if (m_providers.empty())
        return 0;

    return m_providers[0]->GetCoreCount() + 1;
}