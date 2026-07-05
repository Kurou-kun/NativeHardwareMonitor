#include "Providers/Nvml/NvmlProvider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

#include <windows.h>
#include <cstring>
#include <cstdio>

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

    // Optional — not every driver/architecture exports these; skip silently if missing
    m_GetFanRPM      = (decltype(m_GetFanRPM))GetProcAddress(m_module, "nvmlDeviceGetFanSpeedRPM");
    m_GetFieldValues = (decltype(m_GetFieldValues))GetProcAddress(m_module, "nvmlDeviceGetFieldValues");
    m_GetMaxClock    = (decltype(m_GetMaxClock))GetProcAddress(m_module, "nvmlDeviceGetMaxClockInfo");
    m_GetPcieGen     = (decltype(m_GetPcieGen))GetProcAddress(m_module, "nvmlDeviceGetCurrPcieLinkGeneration");
    m_GetPcieWidth   = (decltype(m_GetPcieWidth))GetProcAddress(m_module, "nvmlDeviceGetCurrPcieLinkWidth");
    m_GetEncoderUtil = (decltype(m_GetEncoderUtil))GetProcAddress(m_module, "nvmlDeviceGetEncoderUtilization");
    m_GetDecoderUtil = (decltype(m_GetDecoderUtil))GetProcAddress(m_module, "nvmlDeviceGetDecoderUtilization");
    m_GetPState      = (decltype(m_GetPState))GetProcAddress(m_module, "nvmlDeviceGetPerformanceState");
    m_GetThrottle    = (GetThrottle_t)GetProcAddress(m_module, "nvmlDeviceGetCurrentClocksThrottleReasons");

    m_GetName          = (decltype(m_GetName))GetProcAddress(m_module, "nvmlDeviceGetName");
    m_GetDriverVersion = (decltype(m_GetDriverVersion))GetProcAddress(m_module, "nvmlSystemGetDriverVersion");
    m_GetVbios         = (decltype(m_GetVbios))GetProcAddress(m_module, "nvmlDeviceGetVbiosVersion");
    m_GetPciInfo       = (decltype(m_GetPciInfo))GetProcAddress(m_module, "nvmlDeviceGetPciInfo_v3");

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

    // Fan RPM (Maxwell+ only)
    if (m_GetFanRPM)
    {
        nvmlFanSpeedInfo_t info{};
        info.version = nvmlFanSpeedInfo_v1;
        info.fan = 0;
        if (m_GetFanRPM(dev, &info) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::FanSpeedRPM), info.speed);
    }

    // Memory temperature — not exposed by nvmlDeviceGetTemperature, only via field values
    if (m_GetFieldValues)
    {
        nvmlFieldValue_t field{};
        field.fieldId = NVML_FI_DEV_MEMORY_TEMP;
        if (m_GetFieldValues(dev, 1, &field) == NVML_SUCCESS && field.nvmlReturn == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::MemoryTemperature), static_cast<double>(field.value.uiVal));
    }

    // Max core clock (MHz → Hz)
    if (m_GetMaxClock)
    {
        unsigned int clock = 0;
        if (m_GetMaxClock(dev, NVML_CLOCK_GRAPHICS, &clock) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::MaxCoreClock), clock * 1000000.0);
    }

    // PCIe link (current negotiated generation + width)
    if (m_GetPcieGen)
    {
        unsigned int gen = 0;
        if (m_GetPcieGen(dev, &gen) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::PcieLinkGen), gen);
    }
    if (m_GetPcieWidth)
    {
        unsigned int width = 0;
        if (m_GetPcieWidth(dev, &width) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::PcieLinkWidth), width);
    }

    // NVENC / NVDEC utilization (%). Second out-param is the sampling period, unused.
    if (m_GetEncoderUtil)
    {
        unsigned int util = 0, period = 0;
        if (m_GetEncoderUtil(dev, &util, &period) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::EncoderUsage), util);
    }
    if (m_GetDecoderUtil)
    {
        unsigned int util = 0, period = 0;
        if (m_GetDecoderUtil(dev, &util, &period) == NVML_SUCCESS)
            snap.Set(static_cast<uint32_t>(GpuMetric::DecoderUsage), util);
    }

    // Performance state P0..P15 (P0 = max perf). 32 = unknown → leave unsupported.
    if (m_GetPState)
    {
        nvmlPstates_t pstate = NVML_PSTATE_UNKNOWN;
        if (m_GetPState(dev, &pstate) == NVML_SUCCESS && pstate != NVML_PSTATE_UNKNOWN)
            snap.Set(static_cast<uint32_t>(GpuMetric::PerfState), static_cast<double>(pstate));
    }
}

bool NvmlProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_devices.size())
        return false;

    nvmlDevice_t dev = m_devices[deviceIndex];
    auto metric = static_cast<GpuMetric>(metricId);

    char buf[256] = {};

    switch (metric)
    {
    case GpuMetric::Name:
        if (!m_GetName || m_GetName(dev, buf, sizeof(buf)) != NVML_SUCCESS)
            return false;
        break;

    case GpuMetric::DriverVersion:
        // System-wide, not per-GPU — fine for the common single-vendor-dGPU case
        if (!m_GetDriverVersion || m_GetDriverVersion(buf, sizeof(buf)) != NVML_SUCCESS)
            return false;
        break;

    case GpuMetric::VbiosVersion:
        if (!m_GetVbios || m_GetVbios(dev, buf, sizeof(buf)) != NVML_SUCCESS)
            return false;
        break;

    case GpuMetric::PciDeviceId:
    {
        if (!m_GetPciInfo)
            return false;
        nvmlPciInfo_t pci{};
        if (m_GetPciInfo(dev, &pci) != NVML_SUCCESS)
            return false;
        wchar_t idBuf[16];
        swprintf_s(idBuf, L"%04X:%04X", pci.pciDeviceId & 0xFFFFu, (pci.pciDeviceId >> 16) & 0xFFFFu);
        out = idBuf;
        return true;
    }

    case GpuMetric::ThrottleReasons:
    {
        if (!m_GetThrottle)
            return false;
        unsigned long long reasons = 0;
        if (m_GetThrottle(dev, &reasons) != NVML_SUCCESS)
            return false;

        // GpuIdle alone isn't really "throttling" — report it as the idle state.
        if (reasons == nvmlClocksEventReasonNone || reasons == nvmlClocksEventReasonGpuIdle)
        {
            out = (reasons == nvmlClocksEventReasonGpuIdle) ? L"Idle" : L"None";
            return true;
        }

        struct { unsigned long long bit; const wchar_t* label; } map[] = {
            { nvmlClocksEventReasonSwPowerCap,           L"Power Limit" },
            { nvmlClocksThrottleReasonHwPowerBrakeSlowdown, L"Power Brake" },
            { nvmlClocksThrottleReasonHwThermalSlowdown, L"Thermal (HW)" },
            { nvmlClocksEventReasonSwThermalSlowdown,    L"Thermal (SW)" },
            { nvmlClocksThrottleReasonHwSlowdown,        L"HW Slowdown" },
            { nvmlClocksEventReasonApplicationsClocksSetting, L"App Clock Limit" },
            { nvmlClocksEventReasonDisplayClockSetting,  L"Display Clock" },
            { nvmlClocksEventReasonSyncBoost,            L"Sync Boost" },
        };

        for (auto& m : map)
            if (reasons & m.bit)
            {
                if (!out.empty()) out += L", ";
                out += m.label;
            }

        if (out.empty()) out = L"None"; // unmapped reason bits only
        return true;
    }

    default:
        return false;
    }

    out = std::wstring(buf, buf + strnlen_s(buf, sizeof(buf)));
    return true;
}
