#include "Categories/CPU/CpuBackend.h"

#include "Categories/CPU/WinApiProvider.h"
#include "Utils/Debug.h"

bool CpuBackend::OnInitialize()
{
    auto provider = std::make_unique<WinApiProvider>();

    if (provider->Initialize())
    {
        uint32_t cores = provider->GetCoreCount();

        m_deviceCount = cores + 1;

        m_provider = provider.get();

        m_providers.push_back(std::move(provider));

        LOG_INFO(L"CPU logical cores detected: %u", cores);
    }

    return m_provider != nullptr;
}

void CpuBackend::OnUpdate()
{
    if (m_provider)
        m_provider->Update();
}

double CpuBackend::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    if (!m_provider)
        return 0.0;

    if (deviceIndex >= m_deviceCount)
        return 0.0;

    double value = 0.0;

    switch (static_cast<CpuMetric>(metricId))
    {
    case CpuMetric::Usage:
    {
        if (deviceIndex == 0)
        {
            if (m_provider->GetTotalUsage(value))
                return value;
        }
        else
        {
            if (m_provider->GetCoreUsage(deviceIndex - 1, value))
                return value;
        }

        break;
    }

    case CpuMetric::Clock:
    {
        if (deviceIndex == 0)
        {
            if (m_provider->GetClock(value))
                return value;
        }
        else
        {
            if (m_provider->GetCoreClock(deviceIndex - 1, value))
                return value;
        }

        break;
    }

    case CpuMetric::Temperature:
    {
        if (m_provider->GetTemperature(value))
            return value;

        if (!m_loggedUnsupportedTemp)
        {
            LOG_INFO(L"CPU temperature unsupported");
            m_loggedUnsupportedTemp = true;
        }

        break;
    }

    default:
    {
        if (!m_loggedUnknownMetric)
        {
            LOG_INFO(L"CPU unknown metric requested");
            m_loggedUnknownMetric = true;
        }

        break;
    }
    }

    return 0.0;
}