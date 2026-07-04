#include "Providers/Nvapi/NvapiProvider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

#include <cstring>
#include <cstdio>

// nvapi.h is included only for struct/type definitions.
// Functions are loaded via nvapi_QueryInterface — no nvapi64.lib required.
#pragma warning(push)
#pragma warning(disable: 4996) // suppress deprecation on NvAPI_GPU_GetMemoryInfo declaration
#include "nvapi.h"
#pragma warning(pop)

// IDs from nvapi_interface.h
static constexpr unsigned int ID_Initialize             = 0x0150e828u;
static constexpr unsigned int ID_Unload                 = 0xd22bdd7eu;
static constexpr unsigned int ID_EnumPhysicalGPUs       = 0xe5ac921fu;
static constexpr unsigned int ID_GetThermalSettings     = 0xe3640a56u;
static constexpr unsigned int ID_GetAllClockFrequencies = 0xdcb616c3u;
static constexpr unsigned int ID_GetDynamicPstatesInfoEx= 0x60ded2edu;
static constexpr unsigned int ID_GetMemoryInfo          = 0x07f9b368u;
static constexpr unsigned int ID_GetTachReading         = 0x5f608315u;
static constexpr unsigned int ID_GetFullName             = 0xceee8e9fu;
static constexpr unsigned int ID_GetDriverAndBranchVer   = 0x2926aaadu;
static constexpr unsigned int ID_GetVbiosVersionString   = 0xa561fd7du;
static constexpr unsigned int ID_GetPCIIdentifiers       = 0x2ddfb66eu;

using FnInit      = NvAPI_Status (__cdecl*)();
using FnUnload    = NvAPI_Status (__cdecl*)();
using FnEnumGPUs  = NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32*);
using FnGetTherm  = NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle, NvU32, NV_GPU_THERMAL_SETTINGS*);
using FnGetClocks = NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle, NV_GPU_CLOCK_FREQUENCIES*);
using FnGetPstates= NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle, NV_GPU_DYNAMIC_PSTATES_INFO_EX*);
using FnGetMemory = NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle, NV_DISPLAY_DRIVER_MEMORY_INFO*);
using FnGetTach   = NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle, NvU32*);
using FnGetFullName     = NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle, NvAPI_ShortString);
using FnGetDriverBranch = NvAPI_Status (__cdecl*)(NvU32*, NvAPI_ShortString);
using FnGetVbios        = NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle, NvAPI_ShortString);
using FnGetPciIds       = NvAPI_Status (__cdecl*)(NvPhysicalGpuHandle, NvU32*, NvU32*, NvU32*, NvU32*);

template<typename T>
static T QI(void* (__cdecl* qi)(unsigned int), unsigned int id)
{
    return reinterpret_cast<T>(qi(id));
}

bool NvapiProvider::Initialize()
{
    m_module = LoadLibraryW(L"nvapi64.dll");
    if (!m_module)
    {
        LOG_STARTUP(L"NvapiProvider: nvapi64.dll not found");
        return false;
    }

    m_QueryInterface = reinterpret_cast<decltype(m_QueryInterface)>(
        GetProcAddress(m_module, "nvapi_QueryInterface"));
    if (!m_QueryInterface)
    {
        LOG_STARTUP(L"NvapiProvider: nvapi_QueryInterface not found");
        Shutdown();
        return false;
    }

    if (!LoadFunctions())
    {
        LOG_STARTUP(L"NvapiProvider: required functions unavailable");
        Shutdown();
        return false;
    }

    if (QI<FnInit>(m_QueryInterface, ID_Initialize)() != NVAPI_OK)
    {
        LOG_STARTUP(L"NvapiProvider: NvAPI_Initialize failed");
        Shutdown();
        return false;
    }

    NvU32 count = 0;
    auto  gpus  = reinterpret_cast<NvPhysicalGpuHandle*>(m_gpus);
    if (QI<FnEnumGPUs>(m_QueryInterface, ID_EnumPhysicalGPUs)(gpus, &count) != NVAPI_OK || count == 0)
    {
        LOG_STARTUP(L"NvapiProvider: no NVIDIA GPUs found");
        QI<FnUnload>(m_QueryInterface, ID_Unload)();
        Shutdown();
        return false;
    }

    m_count = static_cast<uint32_t>(count);
    LOG_STARTUP(L"NvapiProvider: initialized (%u device(s))", m_count);
    return true;
}

bool NvapiProvider::LoadFunctions()
{
    m_Init       = m_QueryInterface(ID_Initialize);
    m_Unload     = m_QueryInterface(ID_Unload);
    m_EnumGPUs   = m_QueryInterface(ID_EnumPhysicalGPUs);
    m_GetTherm   = m_QueryInterface(ID_GetThermalSettings);
    m_GetClocks  = m_QueryInterface(ID_GetAllClockFrequencies);
    m_GetPstates = m_QueryInterface(ID_GetDynamicPstatesInfoEx);
    m_GetMemory  = m_QueryInterface(ID_GetMemoryInfo);

    // Optional — not present on fanless GPUs or older drivers; must not fail Initialize()
    m_GetTach    = m_QueryInterface(ID_GetTachReading);

    // Optional string info — absent on very old drivers; must not fail Initialize()
    m_GetFullName     = m_QueryInterface(ID_GetFullName);
    m_GetDriverBranch = m_QueryInterface(ID_GetDriverAndBranchVer);
    m_GetVbios        = m_QueryInterface(ID_GetVbiosVersionString);
    m_GetPciIds       = m_QueryInterface(ID_GetPCIIdentifiers);

    return m_Init && m_Unload && m_EnumGPUs &&
           m_GetTherm && m_GetClocks && m_GetPstates && m_GetMemory;
}

void NvapiProvider::Shutdown()
{
    if (m_Unload && m_QueryInterface)
        QI<FnUnload>(m_QueryInterface, ID_Unload)();
    if (m_module) { FreeLibrary(m_module); m_module = nullptr; }
    m_QueryInterface = nullptr;
    m_Unload         = nullptr;
    m_count          = 0;
}

uint32_t NvapiProvider::GetDeviceCount() const
{
    return m_count;
}

void NvapiProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex >= m_count)
        return;

    auto gpu = reinterpret_cast<NvPhysicalGpuHandle*>(m_gpus)[deviceIndex];

    // Usage
    {
        NV_GPU_DYNAMIC_PSTATES_INFO_EX ps{};
        ps.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;
        if (QI<FnGetPstates>(m_QueryInterface, ID_GetDynamicPstatesInfoEx)(gpu, &ps) == NVAPI_OK
            && ps.utilization[0].bIsPresent) // domain 0 = GPU core utilization
        {
            snap.Set(static_cast<uint32_t>(GpuMetric::Usage),
                     static_cast<double>(ps.utilization[0].percentage));
        }
    }

    // Temperature — scan sensors for GPU core + memory (no hotspot target in public NVAPI)
    {
        NV_GPU_THERMAL_SETTINGS therm{};
        therm.version = NV_GPU_THERMAL_SETTINGS_VER;
        if (QI<FnGetTherm>(m_QueryInterface, ID_GetThermalSettings)(gpu, NVAPI_THERMAL_TARGET_ALL, &therm) == NVAPI_OK)
        {
            for (NvU32 i = 0; i < therm.count && i < NVAPI_MAX_THERMAL_SENSORS_PER_GPU; ++i)
            {
                if (therm.sensor[i].target == NVAPI_THERMAL_TARGET_GPU)
                    snap.Set(static_cast<uint32_t>(GpuMetric::Temperature),
                             static_cast<double>(therm.sensor[i].currentTemp));
                else if (therm.sensor[i].target == NVAPI_THERMAL_TARGET_MEMORY)
                    snap.Set(static_cast<uint32_t>(GpuMetric::MemoryTemperature),
                             static_cast<double>(therm.sensor[i].currentTemp));
            }
        }
    }

    // Fan RPM
    if (m_GetTach)
    {
        NvU32 rpm = 0;
        if (QI<FnGetTach>(m_QueryInterface, ID_GetTachReading)(gpu, &rpm) == NVAPI_OK)
            snap.Set(static_cast<uint32_t>(GpuMetric::FanSpeedRPM), static_cast<double>(rpm));
    }

    // Clocks (kHz → Hz)
    {
        NV_GPU_CLOCK_FREQUENCIES clk{};
        clk.version   = NV_GPU_CLOCK_FREQUENCIES_VER;
        clk.ClockType = NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ;
        if (QI<FnGetClocks>(m_QueryInterface, ID_GetAllClockFrequencies)(gpu, &clk) == NVAPI_OK)
        {
            if (clk.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].bIsPresent)
                snap.Set(static_cast<uint32_t>(GpuMetric::CoreClock),
                         clk.domain[NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency * 1000.0);

            if (clk.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].bIsPresent)
                snap.Set(static_cast<uint32_t>(GpuMetric::MemoryClock),
                         clk.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY].frequency * 1000.0);
        }
    }

    // VRAM (KB → bytes; used = total − available)
    {
        NV_DISPLAY_DRIVER_MEMORY_INFO mem{};
        mem.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;
        if (QI<FnGetMemory>(m_QueryInterface, ID_GetMemoryInfo)(gpu, &mem) == NVAPI_OK)
        {
            snap.Set(static_cast<uint32_t>(GpuMetric::VramTotal),
                     static_cast<double>(mem.dedicatedVideoMemory) * 1024.0);

            // Guard against underflow: curAvailable can transiently exceed dedicated under pressure
            NvU32 used = (mem.curAvailableDedicatedVideoMemory <= mem.dedicatedVideoMemory)
                ? mem.dedicatedVideoMemory - mem.curAvailableDedicatedVideoMemory : 0u;
            snap.Set(static_cast<uint32_t>(GpuMetric::VramUsed), static_cast<double>(used) * 1024.0);
        }
    }
}

static std::wstring ToWide(const char* s)
{
    return std::wstring(s, s + strnlen_s(s, NVAPI_SHORT_STRING_MAX));
}

bool NvapiProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_count)
        return false;

    auto gpu    = reinterpret_cast<NvPhysicalGpuHandle*>(m_gpus)[deviceIndex];
    auto metric = static_cast<GpuMetric>(metricId);

    NvAPI_ShortString buf = {};

    switch (metric)
    {
    case GpuMetric::Name:
        if (!m_GetFullName || QI<FnGetFullName>(m_QueryInterface, ID_GetFullName)(gpu, buf) != NVAPI_OK)
            return false;
        out = ToWide(buf);
        return true;

    case GpuMetric::DriverVersion:
    {
        // System-wide, not per-GPU — fine for the common single-vendor-dGPU case
        if (!m_GetDriverBranch)
            return false;
        NvU32 version = 0;
        if (QI<FnGetDriverBranch>(m_QueryInterface, ID_GetDriverAndBranchVer)(&version, buf) != NVAPI_OK)
            return false;
        wchar_t verBuf[16];
        swprintf_s(verBuf, L"%u.%02u", version / 100, version % 100);
        out = verBuf;
        return true;
    }

    case GpuMetric::VbiosVersion:
        if (!m_GetVbios || QI<FnGetVbios>(m_QueryInterface, ID_GetVbiosVersionString)(gpu, buf) != NVAPI_OK)
            return false;
        out = ToWide(buf);
        return true;

    case GpuMetric::PciDeviceId:
    {
        if (!m_GetPciIds)
            return false;
        NvU32 deviceId = 0, subSystemId = 0, revisionId = 0, extDeviceId = 0;
        if (QI<FnGetPciIds>(m_QueryInterface, ID_GetPCIIdentifiers)(gpu, &deviceId, &subSystemId, &revisionId, &extDeviceId) != NVAPI_OK)
            return false;
        wchar_t idBuf[16];
        swprintf_s(idBuf, L"10DE:%04X", deviceId & 0xFFFFu); // NVIDIA's PCI vendor id is fixed
        out = idBuf;
        return true;
    }

    default:
        return false;
    }
}
