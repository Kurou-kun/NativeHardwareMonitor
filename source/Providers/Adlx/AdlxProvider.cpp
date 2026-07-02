#include "Providers/Adlx/AdlxProvider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

#include "ADLXHelper.h"

extern ADLXHelper g_ADLX;

bool AdlxProvider::Initialize()
{
    if (g_ADLX.Initialize() != ADLX_OK)
    {
        LOG_STARTUP(L"AdlxProvider: ADLX initialization failed");
        return false;
    }

    m_system = g_ADLX.GetSystemServices();
    if (!m_system)
    {
        LOG_STARTUP(L"AdlxProvider: system services unavailable");
        return false;
    }

    if (m_system->GetPerformanceMonitoringServices(&m_perf) != ADLX_OK || !m_perf)
    {
        LOG_STARTUP(L"AdlxProvider: performance monitoring unavailable");
        return false;
    }

    adlx::IADLXGPUList* gpuList = nullptr;
    if (m_system->GetGPUs(&gpuList) != ADLX_OK || !gpuList)
    {
        LOG_STARTUP(L"AdlxProvider: GPU enumeration failed");
        return false;
    }

    adlx_uint count = gpuList->Size();
    for (adlx_uint i = 0; i < count; ++i)
    {
        adlx::IADLXGPU* gpu = nullptr;
        if (gpuList->At(i, &gpu) == ADLX_OK && gpu)
            m_gpus.push_back(gpu);
    }
    gpuList->Release();

    if (m_gpus.empty())
    {
        LOG_STARTUP(L"AdlxProvider: no AMD GPUs found");
        return false;
    }

    LOG_STARTUP(L"AdlxProvider: initialized (%u device(s))", (uint32_t)m_gpus.size());
    return true;
}

uint32_t AdlxProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_gpus.size());
}

void AdlxProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (!m_perf || deviceIndex >= m_gpus.size())
        return;

    adlx::IADLXGPUMetrics* metrics = nullptr;
    if (m_perf->GetCurrentGPUMetrics(m_gpus[deviceIndex], &metrics) != ADLX_OK || !metrics)
        return;

    adlx_double v = 0;
    adlx_int    i = 0;

    if (metrics->GPUUsage(&v)              == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::Usage),              v);
    if (metrics->GPUTemperature(&v)        == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::Temperature),        v);
    if (metrics->GPUHotspotTemperature(&v) == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::HotspotTemperature), v);
    if (metrics->GPUClockSpeed(&i)         == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::CoreClock),          i);
    if (metrics->GPUVRAMClockSpeed(&i)     == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::MemoryClock),        i);
    if (metrics->GPUPower(&v)              == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::Power),              v);
    if (metrics->GPUFanSpeed(&i)           == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::FanSpeed),           i);

    adlx_int vramMB = 0;
    if (metrics->GPUVRAM(&vramMB)          == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::VramUsed), static_cast<double>(vramMB) * 1024.0 * 1024.0);

    metrics->Release();
}

bool AdlxProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return false;
}
