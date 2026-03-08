#include "Categories/GPU/AmdProvider.h"
#include "Utils/Debug.h"

#include "ADLXHelper.h"

extern ADLXHelper g_ADLX;

bool AmdProvider::Initialize()
{
    LOG_INFO(L"Initializing AMD ADLX provider");

    if (g_ADLX.Initialize() != ADLX_OK)
    {
        LOG_INFO(L"ADLX initialization failed");
        return false;
    }

    m_system = g_ADLX.GetSystemServices();
    if (!m_system)
    {
        LOG_INFO(L"ADLX system services unavailable");
        return false;
    }

    if (m_system->GetPerformanceMonitoringServices(&m_perf) != ADLX_OK || !m_perf)
    {
        LOG_INFO(L"ADLX performance monitoring unavailable");
        return false;
    }

    adlx::IADLXGPUList* gpuList = nullptr;

    if (m_system->GetGPUs(&gpuList) != ADLX_OK || !gpuList)
    {
        LOG_INFO(L"ADLX GPU enumeration failed");
        return false;
    }

    adlx_uint count = gpuList->Size();

    LOG_INFO(L"ADLX detected %u GPU(s)", count);

    for (adlx_uint i = 0; i < count; i++)
    {
        adlx::IADLXGPU* gpu = nullptr;

        if (gpuList->At(i, &gpu) == ADLX_OK && gpu)
        {
            m_gpus.push_back(gpu);
            LOG_INFO(L"ADLX GPU %u registered", i);
        }
    }

    gpuList->Release();

    if (m_gpus.empty())
    {
        LOG_INFO(L"No AMD GPUs detected");
        return false;
    }

    LOG_INFO(L"AMD ADLX provider initialized successfully");

    return true;
}

uint32_t AmdProvider::GetDeviceCount() const
{
    return (uint32_t)m_gpus.size();
}

adlx::IADLXGPUMetrics* AmdProvider::GetMetrics(uint32_t index)
{
    if (!m_perf)
    {
        LOG_INFO(L"ADLX performance service unavailable");
        return nullptr;
    }

    if (index >= m_gpus.size())
    {
        LOG_INFO(L"ADLX invalid GPU index: %u", index);
        return nullptr;
    }

    adlx::IADLXGPUMetrics* metrics = nullptr;

    if (m_perf->GetCurrentGPUMetrics(m_gpus[index], &metrics) != ADLX_OK)
    {
        LOG_INFO(L"ADLX failed to retrieve GPU metrics (GPU %u)", index);
        return nullptr;
    }

    return metrics;
}

bool AmdProvider::GetUsage(uint32_t index, double& value)
{
    auto metrics = GetMetrics(index);
    if (!metrics)
        return false;

    adlx_double usage;

    if (metrics->GPUUsage(&usage) != ADLX_OK)
    {
        LOG_INFO(L"ADLX GPUUsage unsupported");
        metrics->Release();
        return false;
    }

    value = usage;

    metrics->Release();
    return true;
}

bool AmdProvider::GetTemperature(uint32_t index, double& value)
{
    auto metrics = GetMetrics(index);
    if (!metrics)
        return false;

    adlx_double temp;

    if (metrics->GPUTemperature(&temp) != ADLX_OK)
    {
        LOG_INFO(L"ADLX GPUTemperature unsupported");
        metrics->Release();
        return false;
    }

    value = temp;

    metrics->Release();
    return true;
}

bool AmdProvider::GetHotspotTemperature(uint32_t index, double& value)
{
    auto metrics = GetMetrics(index);
    if (!metrics)
        return false;

    adlx_double temp;

    if (metrics->GPUHotspotTemperature(&temp) != ADLX_OK)
    {
        LOG_INFO(L"ADLX GPUHotspotTemperature unsupported");
        metrics->Release();
        return false;
    }

    value = temp;

    metrics->Release();
    return true;
}

bool AmdProvider::GetCoreClock(uint32_t index, double& value)
{
    auto metrics = GetMetrics(index);
    if (!metrics)
        return false;

    adlx_int clock;

    if (metrics->GPUClockSpeed(&clock) != ADLX_OK)
    {
        LOG_INFO(L"ADLX GPUClockSpeed unsupported");
        metrics->Release();
        return false;
    }

    value = (double)clock;

    metrics->Release();
    return true;
}

bool AmdProvider::GetPower(uint32_t index, double& value)
{
    auto metrics = GetMetrics(index);
    if (!metrics)
        return false;

    adlx_double power;

    if (metrics->GPUPower(&power) != ADLX_OK)
    {
        LOG_INFO(L"ADLX GPUPower unsupported");
        metrics->Release();
        return false;
    }

    value = power;

    metrics->Release();
    return true;
}

bool AmdProvider::GetMemoryClock(uint32_t index, double& value)
{
    auto metrics = GetMetrics(index);
    if (!metrics)
        return false;

    adlx_int clock;

    if (metrics->GPUVRAMClockSpeed(&clock) != ADLX_OK)
    {
        LOG_INFO(L"ADLX GPUVRAMClockSpeed unsupported");
        metrics->Release();
        return false;
    }

    value = (double)clock;

    metrics->Release();
    return true;
}

bool AmdProvider::GetFanSpeed(uint32_t index, double& value)
{
    auto metrics = GetMetrics(index);
    if (!metrics)
        return false;

    adlx_int speed = 0;

    if (metrics->GPUFanSpeed(&speed) != ADLX_OK)
    {
        LOG_INFO(L"ADLX GPUFanSpeed unsupported");
        metrics->Release();
        return false;
    }

    value = static_cast<double>(speed);

    metrics->Release();
    return true;
}