#include "Providers/Adl2/Adl2Provider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

#include "adl_sdk.h"

#include <cstdlib>
#include <string>
#include <set>

// ADL requires a malloc callback for internal allocations
static void* __stdcall ADL_Alloc(int size) { return malloc(size); }

using ADL_CONTEXT_HANDLE = void*;

using FnCreate      = int (__stdcall*)(ADL_MAIN_MALLOC_CALLBACK, int, ADL_CONTEXT_HANDLE*);
using FnDestroy     = int (__stdcall*)(ADL_CONTEXT_HANDLE);
using FnNumAdapters = int (__stdcall*)(ADL_CONTEXT_HANDLE, int*);
using FnAdapterInfo = int (__stdcall*)(ADL_CONTEXT_HANDLE, LPAdapterInfo, int);
using FnActivity    = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, ADLPMActivity*);
using FnTemperature = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, int, ADLTemperature*);
using FnFanSpeed    = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, int, ADLFanSpeedValue*);
using FnPMLog       = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, ADLPMLogDataOutput*);
using FnVramUsage   = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, int*);
using FnMemoryInfo2 = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, ADLMemoryInfo2*);
using FnBiosInfo    = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, ADLBiosInfo*);

template<typename T> static T FP(void* p) { return reinterpret_cast<T>(p); }

bool Adl2Provider::Initialize()
{
    m_module = LoadLibraryW(L"atiadlxx.dll");
    if (!m_module)
    {
        LOG_STARTUP(L"Adl2Provider: atiadlxx.dll not found");
        return false;
    }

    if (!LoadFunctions())
    {
        LOG_STARTUP(L"Adl2Provider: required functions missing");
        Shutdown();
        return false;
    }

    if (FP<FnCreate>(m_Create)(ADL_Alloc, 1, &m_ctx) != ADL_OK || !m_ctx)
    {
        LOG_STARTUP(L"Adl2Provider: ADL2_Main_Control_Create failed");
        Shutdown();
        return false;
    }

    EnumerateAdapters();

    if (m_adapterIndices.empty())
    {
        LOG_STARTUP(L"Adl2Provider: no AMD adapters found");
        Shutdown();
        return false;
    }

    LOG_STARTUP(L"Adl2Provider: initialized (%u device(s))", (uint32_t)m_adapterIndices.size());
    return true;
}

bool Adl2Provider::LoadFunctions()
{
#define LOAD(name, member) \
    member = GetProcAddress(m_module, #name); \
    if (!member) return false;

    LOAD(ADL2_Main_Control_Create,              m_Create)
    LOAD(ADL2_Main_Control_Destroy,             m_Destroy)
    LOAD(ADL2_Adapter_NumberOfAdapters_Get,     m_NumAdapters)
    LOAD(ADL2_Adapter_AdapterInfo_Get,          m_AdapterInfo)
    LOAD(ADL2_Overdrive5_CurrentActivity_Get,   m_Activity)
    LOAD(ADL2_Overdrive5_Temperature_Get,       m_Temperature)
    LOAD(ADL2_Overdrive5_FanSpeed_Get,          m_FanSpeed)

#undef LOAD

    // Optional, and the whole point of the AMD work in 2.1.6. Overdrive5 answers usage
    // and the two clocks and almost nothing else on an APU — everything below is where
    // temperature, VRAM, power, voltage, PCIe link and VBIOS actually come from on a
    // machine with no ADLX. A null here means an older driver, not a failure.
    m_PMLog       = GetProcAddress(m_module, "ADL2_New_QueryPMLogData_Get");
    m_VramUsage   = GetProcAddress(m_module, "ADL2_Adapter_VRAMUsage_Get");
    m_MemoryInfo2 = GetProcAddress(m_module, "ADL2_Adapter_MemoryInfo2_Get");
    m_BiosInfo    = GetProcAddress(m_module, "ADL2_Adapter_VideoBiosInfo_Get");

    LOG_STARTUP(L"Adl2Provider: optional calls - PMLog=%s VRAMUsage=%s MemoryInfo2=%s VideoBios=%s",
                m_PMLog ? L"yes" : L"no", m_VramUsage ? L"yes" : L"no",
                m_MemoryInfo2 ? L"yes" : L"no", m_BiosInfo ? L"yes" : L"no");
    return true;
}

void Adl2Provider::EnumerateAdapters()
{
    int count = 0;
    if (FP<FnNumAdapters>(m_NumAdapters)(m_ctx, &count) != ADL_OK || count <= 0)
        return;

    auto* info = static_cast<AdapterInfo*>(malloc(sizeof(AdapterInfo) * count));
    if (!info)
        return;

    if (FP<FnAdapterInfo>(m_AdapterInfo)(m_ctx, info, sizeof(AdapterInfo) * count) != ADL_OK)
    {
        free(info);
        return;
    }

    // Deduplicate by bus number — multiple logical indices can map to the same GPU
    std::set<int> seenBusNumbers;
    for (int i = 0; i < count; ++i)
    {
        // ADL reports AMD's vendor id as the DECIMAL number 1002 (0x3EA), not PCI 0x1002.
        // Comparing against 0x1002 alone skipped every AMD adapter on every machine.
        if (info[i].iVendorID != 1002 && info[i].iVendorID != 0x1002)
            continue;

        // iExist, not ADL2_Adapter_Active_Get: that call reports 0 even for the adapter
        // currently driving the display (measured on a 4060 + Raphael iGPU), so it drops
        // everything. iExist is the physical-existence flag, which is what we want here.
        if (!info[i].iExist)
            continue;

        if (!seenBusNumbers.insert(info[i].iBusNumber).second)
            continue;

        m_adapterIndices.push_back(info[i].iAdapterIndex);

        int nameLen = static_cast<int>(strnlen_s(info[i].strAdapterName, ADL_MAX_PATH));
        m_adapterNames.emplace_back(info[i].strAdapterName, info[i].strAdapterName + nameLen);

        CacheAdapterStrings(info[i].iAdapterIndex, info[i].strPNPString);
    }

    free(info);

    m_throttleStatus.assign(m_adapterIndices.size(), -1);
}

// VBIOS version and PCI id never change while the driver is loaded, so they are read
// once here rather than on every gather.
void Adl2Provider::CacheAdapterStrings(int adapterIndex, const char* pnpString)
{
    std::wstring vbios;
    if (m_BiosInfo)
    {
        ADLBiosInfo bios{};
        if (FP<FnBiosInfo>(m_BiosInfo)(m_ctx, adapterIndex, &bios) == ADL_OK)
        {
            int len = static_cast<int>(strnlen_s(bios.strVersion, ADL_MAX_PATH));
            vbios.assign(bios.strVersion, bios.strVersion + len);
        }
    }
    m_vbiosVersions.push_back(vbios);

    // strPNPString looks like "PCI\VEN_1002&DEV_164E&SUBSYS_...". Report it the way the
    // NVIDIA path does, "1002:164E", so a skin can compare the two without special cases.
    std::wstring pciId;
    if (pnpString)
    {
        std::string pnp(pnpString, strnlen_s(pnpString, ADL_MAX_PATH));
        size_t ven = pnp.find("VEN_");
        size_t dev = pnp.find("DEV_");
        if (ven != std::string::npos && dev != std::string::npos &&
            ven + 8 <= pnp.size() && dev + 8 <= pnp.size())
        {
            std::string id = pnp.substr(ven + 4, 4) + ":" + pnp.substr(dev + 4, 4);
            pciId.assign(id.begin(), id.end());
        }
    }
    m_pciIds.push_back(pciId);
}

void Adl2Provider::Shutdown()
{
    if (m_ctx && m_Destroy)
        FP<FnDestroy>(m_Destroy)(m_ctx);
    m_ctx = nullptr;

    if (m_module) { FreeLibrary(m_module); m_module = nullptr; }
    m_adapterIndices.clear();
    m_adapterNames.clear();
    m_vbiosVersions.clear();
    m_pciIds.clear();
    m_throttleStatus.clear();
}

uint32_t Adl2Provider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_adapterIndices.size());
}

void Adl2Provider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex >= m_adapterIndices.size())
        return;

    int adIdx = m_adapterIndices[deviceIndex];

    // Usage + clocks (iEngineClock/iMemoryClock in 10 kHz units → Hz)
    {
        ADLPMActivity act{};
        act.iSize = sizeof(act);
        if (FP<FnActivity>(m_Activity)(m_ctx, adIdx, &act) == ADL_OK)
        {
            snap.Set(static_cast<uint32_t>(GpuMetric::Usage),      static_cast<double>(act.iActivityPercent));
            snap.Set(static_cast<uint32_t>(GpuMetric::CoreClock),  act.iEngineClock  * 10000.0);
            snap.Set(static_cast<uint32_t>(GpuMetric::MemoryClock),act.iMemoryClock  * 10000.0);
        }
    }

    // Temperature (millidegrees → degrees)
    {
        ADLTemperature temp{};
        temp.iSize = sizeof(temp);
        if (FP<FnTemperature>(m_Temperature)(m_ctx, adIdx, 0, &temp) == ADL_OK)
            snap.Set(static_cast<uint32_t>(GpuMetric::Temperature), temp.iTemperature / 1000.0);
    }

    // Fan speed (%)
    {
        ADLFanSpeedValue fan{};
        fan.iSize      = sizeof(fan);
        fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
        if (FP<FnFanSpeed>(m_FanSpeed)(m_ctx, adIdx, 0, &fan) == ADL_OK)
            snap.Set(static_cast<uint32_t>(GpuMetric::FanSpeed), static_cast<double>(fan.iFanSpeed));
    }

    // VRAM in use (MB → bytes)
    if (m_VramUsage)
    {
        int usedMb = 0;
        if (FP<FnVramUsage>(m_VramUsage)(m_ctx, adIdx, &usedMb) == ADL_OK && usedMb >= 0)
            snap.Set(static_cast<uint32_t>(GpuMetric::VramUsed), usedMb * 1024.0 * 1024.0);
    }

    // VRAM total (already bytes). On an APU this is the carve-out, not system RAM.
    if (m_MemoryInfo2)
    {
        ADLMemoryInfo2 mem{};
        if (FP<FnMemoryInfo2>(m_MemoryInfo2)(m_ctx, adIdx, &mem) == ADL_OK && mem.iMemorySize > 0)
            snap.Set(static_cast<uint32_t>(GpuMetric::VramTotal), static_cast<double>(mem.iMemorySize));
    }

    GatherPMLog(deviceIndex, adIdx, snap);
}

// The modern AMD telemetry block, and the only source for most of these on a machine
// with no ADLX: Overdrive5 gives usage and two clocks, PMLog gives temperature, power,
// voltage, PCIe link and per-engine activity. Measured on a Raphael iGPU, which reports
// 17 of these sensors; an older driver may export no PMLog at all, and every sensor
// carries its own `supported` flag, so each one is optional independently.
//
// Units, matching what the NVML/ADLX paths already report: clocks MHz → Hz, temperature
// °C, power W, voltage mV, current A.
void Adl2Provider::GatherPMLog(uint32_t deviceIndex, int adapterIndex, Snapshot& snap)
{
    if (!m_PMLog)
        return;

    ADLPMLogDataOutput out{};
    if (FP<FnPMLog>(m_PMLog)(m_ctx, adapterIndex, &out) != ADL_OK)
        return;

    auto sensor = [&out](int id, double& value) -> bool
    {
        if (id < 0 || id >= ADL_PMLOG_MAX_SENSORS || !out.sensors[id].supported)
            return false;
        value = static_cast<double>(out.sensors[id].value);
        return true;
    };

    auto set = [&](int id, GpuMetric metric, double scale = 1.0)
    {
        double v = 0.0;
        if (sensor(id, v))
            snap.Set(static_cast<uint32_t>(metric), v * scale);
    };

    set(ADL_PMLOG_CLK_GFXCLK,        GpuMetric::CoreClock,   1000000.0);
    set(ADL_PMLOG_CLK_MEMCLK,        GpuMetric::MemoryClock, 1000000.0);
    set(ADL_PMLOG_INFO_ACTIVITY_GFX, GpuMetric::Usage);
    set(ADL_PMLOG_TEMPERATURE_MEM,   GpuMetric::MemoryTemperature);
    set(ADL_PMLOG_TEMPERATURE_HOTSPOT, GpuMetric::HotspotTemperature);
    set(ADL_PMLOG_FAN_RPM,           GpuMetric::FanSpeedRPM);
    set(ADL_PMLOG_FAN_PERCENTAGE,    GpuMetric::FanSpeed);
    set(ADL_PMLOG_GFX_POWER,         GpuMetric::Power);
    set(ADL_PMLOG_ASIC_POWER,        GpuMetric::TotalBoardPower);
    set(ADL_PMLOG_GFX_VOLTAGE,       GpuMetric::Voltage);
    set(ADL_PMLOG_BUS_SPEED,         GpuMetric::PcieLinkGen);
    set(ADL_PMLOG_BUS_LANES,         GpuMetric::PcieLinkWidth);

    // "The" temperature, in the order a user means it. A discrete card reports EDGE; the
    // Raphael iGPU reports neither EDGE nor HOTSPOT but does report GFX and SOC.
    for (int id : { ADL_PMLOG_TEMPERATURE_EDGE, ADL_PMLOG_TEMPERATURE_GFX,
                    ADL_PMLOG_TEMPERATURE_SOC, ADL_PMLOG_TEMPERATURE_HOTSPOT })
    {
        double v = 0.0;
        if (sensor(id, v))
        {
            snap.Set(static_cast<uint32_t>(GpuMetric::Temperature), v);
            break;
        }
    }

    double throttle = 0.0;
    if (deviceIndex < m_throttleStatus.size())
    {
        m_throttleStatus[deviceIndex] =
            sensor(ADL_PMLOG_THROTTLER_STATUS, throttle) ? static_cast<int>(throttle) : -1;
    }
}

bool Adl2Provider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_adapterNames.size())
        return false;

    switch (static_cast<GpuMetric>(metricId))
    {
    case GpuMetric::Name:
        out = m_adapterNames[deviceIndex];
        return true;

    case GpuMetric::VbiosVersion:
        if (deviceIndex >= m_vbiosVersions.size() || m_vbiosVersions[deviceIndex].empty())
            return false;
        out = m_vbiosVersions[deviceIndex];
        return true;

    case GpuMetric::PciDeviceId:
        if (deviceIndex >= m_pciIds.size() || m_pciIds[deviceIndex].empty())
            return false;
        out = m_pciIds[deviceIndex];
        return true;

    case GpuMetric::ThrottleReasons:
    {
        if (deviceIndex >= m_throttleStatus.size() || m_throttleStatus[deviceIndex] < 0)
            return false;   // no PMLog throttler sensor: say nothing rather than "None"

        int status = m_throttleStatus[deviceIndex];
        if (status == 0)
        {
            out = L"None";
            return true;
        }

        out.clear();
        auto add = [&out](const wchar_t* reason)
        {
            if (!out.empty()) out += L", ";
            out += reason;
        };
        if (status & ADL_PMLOG_THROTTLE_POWER)   add(L"Power");
        if (status & ADL_PMLOG_THROTTLE_THERMAL) add(L"Thermal");
        if (status & ADL_PMLOG_THROTTLE_CURRENT) add(L"Current");
        if (out.empty())
            out = L"Unknown";
        return true;
    }

    default:
        return false;
    }
}
