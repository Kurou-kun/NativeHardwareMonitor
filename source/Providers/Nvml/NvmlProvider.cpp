#include "Providers/Nvml/NvmlProvider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

#include <windows.h>

bool NvmlProvider::Initialize()
{
    m_module = LoadLibraryW(L"nvml.dll");
    if (!m_module)
    {
        LOG_STARTUP(L"NvmlProvider: nvml.dll not found");
        return false;
    }

    if (!LoadFunctions())
    {
        LOG_STARTUP(L"NvmlProvider: required functions missing");
        Shutdown();
        return false;
    }

    if (m_Init() != NVML_SUCCESS)
    {
        LOG_STARTUP(L"NvmlProvider: nvmlInit_v2 failed");
        Shutdown();
        return false;
    }

    unsigned int count = 0;
    if (m_GetCount(&count) != NVML_SUCCESS || count == 0)
    {
        LOG_STARTUP(L"NvmlProvider: no NVIDIA GPUs found");
        Shutdown();
        return false;
    }

    m_devices.resize(count);
    for (unsigned int i = 0; i < count; ++i)
    {
        if (m_GetHandle(i, &m_devices[i]) != NVML_SUCCESS)
        {
            LOG_STARTUP(L"NvmlProvider: failed to get device handle %u", i);
            Shutdown();
            return false;
        }
    }

    LOG_STARTUP(L"NvmlProvider: initialized (%u device(s))", count);
    return true;
}

bool NvmlProvider::LoadFunctions()
{
#define LOAD(name, member) \
    member = (decltype(member))GetProcAddress(m_module, #name); \
    if (!member) return false;

    LOAD(nvmlInit_v2,                       m_Init)
    LOAD(nvmlShutdown,                      m_Shutdown)
    LOAD(nvmlDeviceGetCount_v2,             m_GetCount)
    LOAD(nvmlDeviceGetHandleByIndex_v2,     m_GetHandle)
    LOAD(nvmlDeviceGetUtilizationRates,     m_GetUtil)
    LOAD(nvmlDeviceGetMemoryInfo,           m_GetMemory)
    LOAD(nvmlDeviceGetTemperature,          m_GetTemp)
    LOAD(nvmlDeviceGetClockInfo,            m_GetClock)
    LOAD(nvmlDeviceGetFanSpeed,             m_GetFan)
    LOAD(nvmlDeviceGetPowerUsage,           m_GetPower)
    LOAD(nvmlDeviceGetPowerManagementLimit, m_GetPowerLimit)

#undef LOAD
    return true;
}

void NvmlProvider::Shutdown()
{
    if (m_Shutdown) m_Shutdown();
    if (m_module) { FreeLibrary(m_module); m_module = nullptr; }
}

uint32_t NvmlProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}

void NvmlProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex >= m_devices.size())
        return;

    nvmlDevice_t dev = m_devices[deviceIndex];

    // Usage
    if (m_GetUtil)
    {
        nvmlUtilization_t util;
        if (m_GetUtil(dev, &util) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::Usage), util.gpu);
    }

    // VRAM
    if (m_GetMemory)
    {
        nvmlMemory_t mem{};
        if (m_GetMemory(dev, &mem) == NVML_SUCCESS)
        {
            snap.Set(static_cast<uint32_t>(GpuMetric::VramUsed),  static_cast<double>(mem.used));
            snap.Set(static_cast<uint32_t>(GpuMetric::VramTotal), static_cast<double>(mem.total));
        }
    }

    // Temperature
    if (m_GetTemp)
    {
        unsigned int temp = 0;
        if (m_GetTemp(dev, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::Temperature), temp);
    }

    // Clocks (MHz → Hz)
    if (m_GetClock)
    {
        unsigned int clock = 0;
        if (m_GetClock(dev, NVML_CLOCK_GRAPHICS, &clock) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::CoreClock), clock * 1000000.0);

        if (m_GetClock(dev, NVML_CLOCK_MEM, &clock) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::MemoryClock), clock * 1000000.0);
    }

    // Fan
    if (m_GetFan)
    {
        unsigned int speed = 0;
        if (m_GetFan(dev, &speed) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::FanSpeed), speed);
    }

    // Power (mW → W)
    if (m_GetPower)
    {
        unsigned int power = 0;
        if (m_GetPower(dev, &power) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::Power), power / 1000.0);
    }

    if (m_GetPowerLimit)
    {
        unsigned int limit = 0;
        if (m_GetPowerLimit(dev, &limit) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::PowerLimit), limit / 1000.0);
    }
}

bool NvmlProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return false;
}
