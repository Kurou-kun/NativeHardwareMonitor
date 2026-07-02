#include "Providers/Adl2/Adl2Provider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

#include "adl_sdk.h"

#include <cstdlib>
#include <set>

// ADL requires a malloc callback for internal allocations
static void* __stdcall ADL_Alloc(int size) { return malloc(size); }

using ADL_CONTEXT_HANDLE = void*;

using FnCreate      = int (__stdcall*)(ADL_MAIN_MALLOC_CALLBACK, int, ADL_CONTEXT_HANDLE*);
using FnDestroy     = int (__stdcall*)(ADL_CONTEXT_HANDLE);
using FnNumAdapters = int (__stdcall*)(ADL_CONTEXT_HANDLE, int*);
using FnAdapterInfo = int (__stdcall*)(ADL_CONTEXT_HANDLE, LPAdapterInfo, int);
using FnActive      = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, int*);
using FnActivity    = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, ADLPMActivity*);
using FnTemperature = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, int, ADLTemperature*);
using FnFanSpeed    = int (__stdcall*)(ADL_CONTEXT_HANDLE, int, int, ADLFanSpeedValue*);

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
        LOG_STARTUP(L"Adl2Provider: no active AMD adapters found");
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
    LOAD(ADL2_Adapter_Active_Get,               m_Active)
    LOAD(ADL2_Overdrive5_CurrentActivity_Get,   m_Activity)
    LOAD(ADL2_Overdrive5_Temperature_Get,       m_Temperature)
    LOAD(ADL2_Overdrive5_FanSpeed_Get,          m_FanSpeed)

#undef LOAD
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
        if (info[i].iVendorID != 0x1002) // AMD vendor ID
            continue;

        int isActive = 0;
        if (FP<FnActive>(m_Active)(m_ctx, info[i].iAdapterIndex, &isActive) != ADL_OK || !isActive)
            continue;

        if (!seenBusNumbers.insert(info[i].iBusNumber).second)
            continue;

        m_adapterIndices.push_back(info[i].iAdapterIndex);
    }

    free(info);
}

void Adl2Provider::Shutdown()
{
    if (m_ctx && m_Destroy)
        FP<FnDestroy>(m_Destroy)(m_ctx);
    m_ctx = nullptr;

    if (m_module) { FreeLibrary(m_module); m_module = nullptr; }
    m_adapterIndices.clear();
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
}

bool Adl2Provider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return false;
}
