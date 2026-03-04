#include "AmdProvider.h"

using namespace adlx;

AmdProvider::AmdProvider()
{
}

AmdProvider::~AmdProvider()
{
    m_loader.Shutdown();
}

bool AmdProvider::Initialize()
{
    if (!m_loader.Initialize())
        return false;

    m_initialized = true;
    return true;
}

uint32_t AmdProvider::GetDeviceCount() const
{
    if (!m_initialized)
        return 0;

    IADLXSystem* system = m_loader.GetSystemServices();
    if (!system)
        return 0;

    IADLXGPUList* list = nullptr;
    if (system->GetGPUs(&list) != ADLX_OK || !list)
        return 0;

    adlx_uint count = list->Size();
    list->Release();

    return static_cast<uint32_t>(count);
}

bool AmdProvider::GetUtilization(uint32_t index, double& value)
{
    value = 0.0;

    if (!m_initialized)
        return false;

    IADLXSystem* system = m_loader.GetSystemServices();
    if (!system)
        return false;

    IADLXGPUList* list = nullptr;
    if (system->GetGPUs(&list) != ADLX_OK || !list)
        return false;

    if (index >= list->Size())
    {
        list->Release();
        return false;
    }

    IADLXGPU* gpu = nullptr;
    if (list->At(index, &gpu) != ADLX_OK || !gpu)
    {
        list->Release();
        return false;
    }

    IADLXPerformanceMonitoringServices* perf = nullptr;
    if (system->GetPerformanceMonitoringServices(&perf) != ADLX_OK || !perf)
    {
        gpu->Release();
        list->Release();
        return false;
    }

    perf->StartPerformanceMetricsTracking();

    IADLXGPUMetrics* metrics = nullptr;
    if (perf->GetCurrentGPUMetrics(gpu, &metrics) != ADLX_OK || !metrics)
    {
        perf->Release();
        gpu->Release();
        list->Release();
        return false;
    }

    adlx_double usage = 0;
    if (metrics->GPUUsage(&usage) == ADLX_OK)
        value = usage;

    metrics->Release();
    perf->Release();
    gpu->Release();
    list->Release();

    return true;
}

bool AmdProvider::GetMemoryUsed(uint32_t index, uint64_t& value)
{
    value = 0;

    if (!m_initialized)
        return false;

    IADLXSystem* system = m_loader.GetSystemServices();
    if (!system)
        return false;

    IADLXGPUList* list = nullptr;
    if (system->GetGPUs(&list) != ADLX_OK || !list)
        return false;

    if (index >= list->Size())
    {
        list->Release();
        return false;
    }

    IADLXGPU* gpu = nullptr;
    if (list->At(index, &gpu) != ADLX_OK || !gpu)
    {
        list->Release();
        return false;
    }

    IADLXPerformanceMonitoringServices* perf = nullptr;
    if (system->GetPerformanceMonitoringServices(&perf) != ADLX_OK || !perf)
    {
        gpu->Release();
        list->Release();
        return false;
    }

    perf->StartPerformanceMetricsTracking();

    IADLXGPUMetrics* metrics = nullptr;
    if (perf->GetCurrentGPUMetrics(gpu, &metrics) != ADLX_OK || !metrics)
    {
        perf->Release();
        gpu->Release();
        list->Release();
        return false;
    }

    adlx_int mem = 0;
    if (metrics->GPUDedicatedVRAM(&mem) == ADLX_OK)
        value = static_cast<uint64_t>(mem);

    metrics->Release();
    perf->Release();
    gpu->Release();
    list->Release();

    return true;
}