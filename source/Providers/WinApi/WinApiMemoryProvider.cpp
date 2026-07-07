#include "Providers/WinApi/WinApiMemoryProvider.h"
#include "Types/MemoryMetric.h"
#include "Utils/Debug.h"
#include "Utils/WmiUtil.h"

#include <psapi.h>
#include <comdef.h>

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
    // Connection kept alive across gathers (see header) — QueryPageFileUsage reuses it.
    m_wmiServices = Wmi::Connect(L"ROOT\\CIMV2", m_comInitialized);
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
        double used = 0.0, total = 0.0; // absent fields stay 0 — a pagefile can report either
        Wmi::GetU32(obj, L"CurrentUsage", used);
        Wmi::GetU32(obj, L"AllocatedBaseSize", total);
        usedMb  += used;
        totalMb += total;

        found = true;
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
            double configured = 0.0, rated = 0.0, smbiosType = 0.0;
            Wmi::GetU32(obj, L"ConfiguredClockSpeed", configured);
            Wmi::GetU32(obj, L"Speed", rated);
            Wmi::GetU32(obj, L"SMBIOSMemoryType", smbiosType);

            m_ramSpeed     = configured > 0 ? configured : rated; // MT/s
            m_memoryType   = SmbiosMemoryTypeToString(static_cast<long>(smbiosType));
            m_manufacturer = Wmi::GetStr(obj, L"Manufacturer");
            m_partNumber   = Wmi::GetStr(obj, L"PartNumber");
        }

        obj->Release();
        obj = nullptr;
    }
    enumerator->Release();
}
