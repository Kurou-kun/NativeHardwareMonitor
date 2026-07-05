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
    ReadPhysicalMemory(); // best-effort RAM identity; stays unsupported if WMI is unavailable

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

        // Static RAM identity (numeric parts) — physical-RAM properties, device 0 only.
        if (m_ramSpeed > 0.0)    snap.Set(static_cast<uint32_t>(MemoryMetric::Speed),       m_ramSpeed);
        if (m_moduleCount > 0)   snap.Set(static_cast<uint32_t>(MemoryMetric::ModuleCount), m_moduleCount);
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
    if (deviceIndex != 0)
        return false;

    switch (static_cast<MemoryMetric>(metricId))
    {
    case MemoryMetric::MemoryType:   if (m_memoryType.empty())   return false; out = m_memoryType;   return true;
    case MemoryMetric::Manufacturer: if (m_manufacturer.empty()) return false; out = m_manufacturer; return true;
    case MemoryMetric::PartNumber:   if (m_partNumber.empty())   return false; out = m_partNumber;   return true;
    default:                         return false;
    }
}

static const wchar_t* SmbiosMemoryTypeToString(long type)
{
    // SMBIOS spec 7.18.2 memory type values.
    switch (type)
    {
    case 0x12: return L"DDR";
    case 0x13: return L"DDR2";
    case 0x18: return L"DDR3";
    case 0x1A: return L"DDR4";
    case 0x1E: return L"LPDDR4";
    case 0x22: return L"DDR5";
    case 0x23: return L"LPDDR5";
    default:   return L"";
    }
}

static std::wstring TrimBstr(const wchar_t* s)
{
    if (!s) return L"";
    std::wstring v = s;
    size_t start = v.find_first_not_of(L' ');
    size_t end   = v.find_last_not_of(L' ');
    return start == std::wstring::npos ? L"" : v.substr(start, end - start + 1);
}

// Win32_PhysicalMemory: static RAM hardware identity, read once. Modules are near
// always identical, so report the first instance's fields + a module count.
void WinApiMemoryProvider::ReadPhysicalMemory()
{
    if (!m_wmiServices)
        return;

    IEnumWbemClassObject* enumerator = nullptr;
    HRESULT hr = m_wmiServices->ExecQuery(_bstr_t(L"WQL"),
        _bstr_t(L"SELECT ConfiguredClockSpeed, Speed, SMBIOSMemoryType, Manufacturer, PartNumber FROM Win32_PhysicalMemory"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);

    if (FAILED(hr) || !enumerator)
        return;

    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;
    while (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK && obj)
    {
        ++m_moduleCount;

        if (m_moduleCount == 1) // take identity from the first module
        {
            auto getU32 = [&](const wchar_t* name) -> long {
                VARIANT v; VariantInit(&v);
                long out = 0;
                if (SUCCEEDED(obj->Get(name, 0, &v, nullptr, nullptr)))
                {
                    if      (v.vt == VT_I4)  out = v.lVal;
                    else if (v.vt == VT_UI4) out = static_cast<long>(v.ulVal);
                }
                VariantClear(&v);
                return out;
            };
            auto getStr = [&](const wchar_t* name) -> std::wstring {
                VARIANT v; VariantInit(&v);
                std::wstring out;
                if (SUCCEEDED(obj->Get(name, 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR)
                    out = TrimBstr(v.bstrVal);
                VariantClear(&v);
                return out;
            };

            long configured = getU32(L"ConfiguredClockSpeed");
            long rated       = getU32(L"Speed");
            m_ramSpeed       = static_cast<double>(configured > 0 ? configured : rated); // MT/s

            m_memoryType   = SmbiosMemoryTypeToString(getU32(L"SMBIOSMemoryType"));
            m_manufacturer = getStr(L"Manufacturer");
            m_partNumber   = getStr(L"PartNumber");
        }

        obj->Release();
        obj = nullptr;
    }
    enumerator->Release();
}
