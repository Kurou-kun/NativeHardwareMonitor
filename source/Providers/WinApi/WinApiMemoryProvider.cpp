#include "Providers/WinApi/WinApiMemoryProvider.h"
#include "Types/MemoryMetric.h"
#include "Utils/Debug.h"

#include <psapi.h>
#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")

bool WinApiMemoryProvider::Initialize()
{
    m_status.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&m_status))
    {
        LOG_STARTUP(L"WinApiMemoryProvider: GlobalMemoryStatusEx failed");
        return false;
    }

    InitWmi(); // best-effort — Swap device just reports unsupported if this fails

    LOG_STARTUP(L"WinApiMemoryProvider: initialized");
    return true;
}

WinApiMemoryProvider::~WinApiMemoryProvider()
{
    if (m_wmiServices) m_wmiServices->Release();
    if (m_comInitialized) CoUninitialize();
}

void WinApiMemoryProvider::InitWmi()
{
    // RPC_E_CHANGED_MODE means the calling thread already has COM initialized in a
    // different apartment (Rainmeter's own thread does) — that's not fatal, WMI works
    // fine in either apartment; we just must not call CoUninitialize for one we didn't init.
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    m_comInitialized = SUCCEEDED(coInit);

    IWbemLocator* locator = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                 IID_IWbemLocator, (LPVOID*)&locator)) || !locator)
        return;

    HRESULT hr = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr,
                                         0, nullptr, nullptr, &m_wmiServices);
    if (FAILED(hr) || !m_wmiServices)
    {
        m_wmiServices = nullptr;
        locator->Release();
        return;
    }

    CoSetProxyBlanket(m_wmiServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                       RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    locator->Release();
}

bool WinApiMemoryProvider::QueryPageFileUsage(double& usedBytes, double& totalBytes)
{
    if (!m_wmiServices)
        return false;

    IEnumWbemClassObject* enumerator = nullptr;
    HRESULT hr = m_wmiServices->ExecQuery(_bstr_t(L"WQL"),
        _bstr_t(L"SELECT CurrentUsage, AllocatedBaseSize FROM Win32_PageFileUsage"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);

    if (FAILED(hr) || !enumerator)
        return false;

    double usedMb = 0.0, totalMb = 0.0;
    bool   found  = false;

    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;
    while (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK && obj)
    {
        VARIANT used, total;
        VariantInit(&used);
        VariantInit(&total);

        if (SUCCEEDED(obj->Get(L"CurrentUsage", 0, &used, nullptr, nullptr)) && used.vt == VT_I4)
            usedMb += used.lVal;
        if (SUCCEEDED(obj->Get(L"AllocatedBaseSize", 0, &total, nullptr, nullptr)) && total.vt == VT_I4)
            totalMb += total.lVal;

        found = true;

        VariantClear(&used);
        VariantClear(&total);
        obj->Release();
    }
    enumerator->Release();

    if (!found)
        return false; // no pagefile configured — genuinely unsupported, not an error

    usedBytes  = usedMb  * 1024.0 * 1024.0;
    totalBytes = totalMb * 1024.0 * 1024.0;
    return true;
}

uint32_t WinApiMemoryProvider::GetDeviceCount() const
{
    return 3; // 0 = RAM (physical), 1 = Virtual (commit charge), 2 = Swap (pagefile.sys on disk)
}

void WinApiMemoryProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex == 0)
    {
        m_status.dwLength = sizeof(MEMORYSTATUSEX);
        if (!GlobalMemoryStatusEx(&m_status))
            return;

        double total = static_cast<double>(m_status.ullTotalPhys);
        double free  = static_cast<double>(m_status.ullAvailPhys);
        double used  = total - free;

        snap.Set(static_cast<uint32_t>(MemoryMetric::Total),       total);
        snap.Set(static_cast<uint32_t>(MemoryMetric::Free),        free);
        snap.Set(static_cast<uint32_t>(MemoryMetric::Used),        used);
        snap.Set(static_cast<uint32_t>(MemoryMetric::UsedPercent), total > 0 ? used / total * 100.0 : 0.0);

        PERFORMANCE_INFORMATION perf{ sizeof(perf) };
        if (GetPerformanceInfo(&perf, sizeof(perf)))
            snap.Set(static_cast<uint32_t>(MemoryMetric::Cached),
                     static_cast<double>(perf.SystemCache) * perf.PageSize);
    }
    else if (deviceIndex == 1)
    {
        // "Virtual memory" in the Windows sense = Commit Charge: what Task Manager
        // labels "Committed" — not the same as physically-installed RAM.
        PERFORMANCE_INFORMATION perf{ sizeof(perf) };
        if (!GetPerformanceInfo(&perf, sizeof(perf)))
            return;

        double total = static_cast<double>(perf.CommitLimit) * perf.PageSize;
        double used  = static_cast<double>(perf.CommitTotal) * perf.PageSize;

        snap.Set(static_cast<uint32_t>(MemoryMetric::Total),       total);
        snap.Set(static_cast<uint32_t>(MemoryMetric::Free),        total - used);
        snap.Set(static_cast<uint32_t>(MemoryMetric::Used),        used);
        snap.Set(static_cast<uint32_t>(MemoryMetric::UsedPercent), total > 0 ? used / total * 100.0 : 0.0);
    }
    else if (deviceIndex == 2)
    {
        // The literal pagefile.sys on disk — distinct from Virtual/Commit Charge above,
        // which is RAM+pagefile combined. Only source for this is WMI.
        double used = 0.0, total = 0.0;
        if (!QueryPageFileUsage(used, total))
            return;

        snap.Set(static_cast<uint32_t>(MemoryMetric::Total),       total);
        snap.Set(static_cast<uint32_t>(MemoryMetric::Free),        total - used);
        snap.Set(static_cast<uint32_t>(MemoryMetric::Used),        used);
        snap.Set(static_cast<uint32_t>(MemoryMetric::UsedPercent), total > 0 ? used / total * 100.0 : 0.0);
    }
}

bool WinApiMemoryProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return false;
}
