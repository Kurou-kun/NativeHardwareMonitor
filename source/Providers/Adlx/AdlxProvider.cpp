#include "Providers/Adlx/AdlxProvider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

#include "ADLXHelper.h"
#include "IPerformanceMonitoring2.h"
#include "ISystem2.h"

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

    m_vramTotal.resize(m_gpus.size(), 0.0);
    for (size_t i = 0; i < m_gpus.size(); ++i)
    {
        adlx_uint vramMB = 0;
        if (m_gpus[i]->TotalVRAM(&vramMB) == ADLX_OK)
            m_vramTotal[i] = static_cast<double>(vramMB) * 1024.0 * 1024.0;
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
    if (metrics->GPUClockSpeed(&i)         == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::CoreClock),          i * 1000000.0); // MHz → Hz, matches NVML/NVAPI/ADL2
    if (metrics->GPUVRAMClockSpeed(&i)     == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::MemoryClock),        i * 1000000.0);
    if (metrics->GPUPower(&v)              == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::Power),              v);
    if (metrics->GPUFanSpeed(&i)           == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::FanSpeed),           i);
    if (metrics->GPUTotalBoardPower(&v)    == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::TotalBoardPower),   v);
    if (metrics->GPUIntakeTemperature(&v)  == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::IntakeTemperature), v);
    if (metrics->GPUVoltage(&i)            == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::Voltage),           i);

    adlx_int vramMB = 0;
    if (metrics->GPUVRAM(&vramMB)          == ADLX_OK) snap.Set(static_cast<uint32_t>(GpuMetric::VramUsed), static_cast<double>(vramMB) * 1024.0 * 1024.0);

    if (deviceIndex < m_vramTotal.size() && m_vramTotal[deviceIndex] > 0.0)
        snap.Set(static_cast<uint32_t>(GpuMetric::VramTotal), m_vramTotal[deviceIndex]);

    // Memory temperature — only on the IADLXGPUMetrics1 extension interface
    adlx::IADLXGPUMetrics1* metrics1 = nullptr;
    if (metrics->QueryInterface(adlx::IADLXGPUMetrics1::IID(), (void**)&metrics1) == ADLX_OK && metrics1)
    {
        adlx_double memTemp = 0;
        if (metrics1->GPUMemoryTemperature(&memTemp) == ADLX_OK)
            snap.Set(static_cast<uint32_t>(GpuMetric::MemoryTemperature), memTemp);
        metrics1->Release();
    }

    metrics->Release();
}

static std::wstring ToWide(const char* s)
{
    return s ? std::wstring(s, s + strlen(s)) : std::wstring();
}

bool AdlxProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_gpus.size())
        return false;

    auto* gpu   = m_gpus[deviceIndex];
    auto metric = static_cast<GpuMetric>(metricId);

    switch (metric)
    {
    case GpuMetric::Name:
    {
        const char* name = nullptr;
        if (gpu->Name(&name) != ADLX_OK || !name)
            return false;
        out = ToWide(name);
        return true;
    }

    case GpuMetric::DriverVersion:
    {
        adlx::IADLXGPU2* gpu2 = nullptr;
        if (gpu->QueryInterface(adlx::IADLXGPU2::IID(), (void**)&gpu2) != ADLX_OK || !gpu2)
            return false;
        const char* version = nullptr;
        bool ok = gpu2->DriverVersion(&version) == ADLX_OK && version;
        if (ok) out = ToWide(version);
        gpu2->Release();
        return ok;
    }

    case GpuMetric::VbiosVersion:
    {
        const char* partNumber = nullptr;
        const char* version    = nullptr;
        const char* date       = nullptr;
        if (gpu->BIOSInfo(&partNumber, &version, &date) != ADLX_OK || !version)
            return false;
        out = ToWide(version);
        return true;
    }

    case GpuMetric::PciDeviceId:
    {
        const char* vendorId = nullptr;
        const char* deviceId = nullptr;
        if (gpu->VendorId(&vendorId) != ADLX_OK || gpu->DeviceId(&deviceId) != ADLX_OK || !vendorId || !deviceId)
            return false;
        out = ToWide(vendorId) + L":" + ToWide(deviceId);
        return true;
    }

    default:
        return false;
    }
}
