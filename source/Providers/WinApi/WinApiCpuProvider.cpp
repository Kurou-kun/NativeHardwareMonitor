#include "Providers/WinApi/WinApiCpuProvider.h"
#include "Types/CpuMetric.h"
#include "Utils/Debug.h"
#include "Utils/WmiUtil.h"

#include <algorithm>
#include <cstdio>
#include <powrprof.h>
#include <powerbase.h>

#pragma comment(lib, "powrprof.lib")

#define SystemProcessorPerformanceInformation 8

// PROCESSOR_POWER_INFORMATION is a real, stable struct returned by
// CallNtPowerInformation(ProcessorInformation, ...) — declared here because the
// public SDK headers declare the function but never define this output struct.
typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION;

bool WinApiCpuProvider::Initialize()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    m_ntQuery = (NtQuerySystemInformation_t)GetProcAddress(ntdll, "NtQuerySystemInformation");
    if (!m_ntQuery) return false;

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    m_coreCount = info.dwNumberOfProcessors;

    m_prevTimes.resize(m_coreCount);
    m_currTimes.resize(m_coreCount);
    m_coreUsage.resize(m_coreCount, 0.0);
    m_coreClock.resize(m_coreCount, 0.0);
    m_coreMaxClock.resize(m_coreCount, 0.0);

    QueryProcessorTimes(m_prevTimes);

    ReadIdentity();
    ReadTopology();
    ReadVoltage();

    LOG_STARTUP(L"WinApiCpuProvider: initialized (%u core(s))", m_coreCount);
    return true;
}

uint32_t WinApiCpuProvider::GetDeviceCount() const
{
    return m_coreCount + 1; // device 0 = total, 1..N = per-core
}

void WinApiCpuProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    // Gather on device 0 (updates all core data)
    if (deviceIndex == 0)
    {
        if (!QueryProcessorTimes(m_currTimes))
            return;

        double totalUsage = 0.0;
        for (uint32_t i = 0; i < m_coreCount; ++i)
        {
            auto& prev = m_prevTimes[i];
            auto& curr = m_currTimes[i];

            long long idle   = curr.IdleTime.QuadPart   - prev.IdleTime.QuadPart;
            long long kernel = curr.KernelTime.QuadPart - prev.KernelTime.QuadPart;
            long long user   = curr.UserTime.QuadPart   - prev.UserTime.QuadPart;
            long long total  = kernel + user;

            m_coreUsage[i] = total > 0 ? (double)(total - idle) / total * 100.0 : 0.0;
            totalUsage += m_coreUsage[i];
        }

        m_totalUsage = totalUsage / m_coreCount;
        m_prevTimes  = m_currTimes;

        // Clock — current + max per logical processor, straight from the OS power API
        std::vector<PROCESSOR_POWER_INFORMATION> ppi(m_coreCount);
        m_clockValid = CallNtPowerInformation(ProcessorInformation, nullptr, 0,
                ppi.data(), sizeof(PROCESSOR_POWER_INFORMATION) * m_coreCount) == 0;

        if (m_clockValid)
        {
            double sumClock = 0.0, peakMax = 0.0;
            for (uint32_t i = 0; i < m_coreCount; ++i)
            {
                m_coreClock[i]    = ppi[i].CurrentMhz * 1000000.0;
                m_coreMaxClock[i] = ppi[i].MaxMhz     * 1000000.0;
                sumClock += m_coreClock[i];
                peakMax   = std::max(peakMax, m_coreMaxClock[i]);
            }
            m_totalClock    = sumClock / m_coreCount;
            m_totalMaxClock = peakMax;
        }

        snap.Set(static_cast<uint32_t>(CpuMetric::Usage), m_totalUsage);

        if (m_clockValid)
        {
            snap.Set(static_cast<uint32_t>(CpuMetric::Clock),    m_totalClock);
            snap.Set(static_cast<uint32_t>(CpuMetric::MaxClock), m_totalMaxClock);
        }

        if (m_voltage >= 0.0)
            snap.Set(static_cast<uint32_t>(CpuMetric::Voltage), m_voltage);

        // Static topology — only meaningful on the aggregate device. Set once each
        // tick (cheap); leave unset (reads -1) when the OS didn't supply the value.
        snap.Set(static_cast<uint32_t>(CpuMetric::ThreadCount), m_coreCount);
        if (m_physicalCores > 0) snap.Set(static_cast<uint32_t>(CpuMetric::CoreCount), m_physicalCores);
        if (m_cacheL1 > 0)       snap.Set(static_cast<uint32_t>(CpuMetric::CacheL1), static_cast<double>(m_cacheL1));
        if (m_cacheL2 > 0)       snap.Set(static_cast<uint32_t>(CpuMetric::CacheL2), static_cast<double>(m_cacheL2));
        if (m_cacheL3 > 0)       snap.Set(static_cast<uint32_t>(CpuMetric::CacheL3), static_cast<double>(m_cacheL3));
    }
    else
    {
        uint32_t coreIdx = deviceIndex - 1;
        if (coreIdx < m_coreUsage.size())
        {
            snap.Set(static_cast<uint32_t>(CpuMetric::Usage), m_coreUsage[coreIdx]);

            if (m_clockValid)
            {
                snap.Set(static_cast<uint32_t>(CpuMetric::Clock),    m_coreClock[coreIdx]);
                snap.Set(static_cast<uint32_t>(CpuMetric::MaxClock), m_coreMaxClock[coreIdx]);
            }
        }
    }
}

bool WinApiCpuProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    switch (static_cast<CpuMetric>(metricId))
    {
    case CpuMetric::Name:             if (m_name.empty())             return false; out = m_name;             return true;
    case CpuMetric::Vendor:           if (m_vendor.empty())           return false; out = m_vendor;           return true;
    case CpuMetric::Identifier:       if (m_identifier.empty())       return false; out = m_identifier;       return true;
    case CpuMetric::MicrocodeVersion: if (m_microcodeVersion.empty()) return false; out = m_microcodeVersion; return true;
    case CpuMetric::Architecture:     if (m_architecture.empty())     return false; out = m_architecture;     return true;
    default:                         return false;
    }
}

bool WinApiCpuProvider::QueryProcessorTimes(std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION>& data)
{
    if (!m_ntQuery) return false;

    ULONG size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * m_coreCount;
    return m_ntQuery(SystemProcessorPerformanceInformation, data.data(), size, nullptr) == 0;
}

static std::wstring ReadTrimmedSz(HKEY key, const wchar_t* valueName)
{
    wchar_t buf[256] = {};
    DWORD   size     = sizeof(buf);
    if (RegQueryValueExW(key, valueName, nullptr, nullptr, (LPBYTE)buf, &size) != ERROR_SUCCESS)
        return L"";

    std::wstring value = buf;
    size_t start = value.find_first_not_of(L' ');
    size_t end   = value.find_last_not_of(L' ');
    return start == std::wstring::npos ? L"" : value.substr(start, end - start + 1);
}

void WinApiCpuProvider::ReadIdentity()
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &key) != ERROR_SUCCESS)
        return;

    m_name       = ReadTrimmedSz(key, L"ProcessorNameString");
    m_vendor     = ReadTrimmedSz(key, L"VendorIdentifier");
    m_identifier = ReadTrimmedSz(key, L"Identifier");

    // "Update Revision" is REG_BINARY; the microcode revision is its last 4 bytes,
    // little-endian — the same convention CPU-Z/HWiNFO read it with.
    BYTE  revBuf[64] = {};
    DWORD revSize    = sizeof(revBuf);
    if (RegQueryValueExW(key, L"Update Revision", nullptr, nullptr, revBuf, &revSize) == ERROR_SUCCESS && revSize >= 4)
    {
        uint32_t revision = revBuf[revSize - 4] | (revBuf[revSize - 3] << 8)
                          | (revBuf[revSize - 2] << 16) | (revBuf[revSize - 1] << 24);
        wchar_t revStr[16];
        swprintf_s(revStr, L"0x%X", revision);
        m_microcodeVersion = revStr;
    }

    RegCloseKey(key);
}

void WinApiCpuProvider::ReadTopology()
{
    // Architecture — driverless, from the native (not WOW64) system info.
    SYSTEM_INFO ni;
    GetNativeSystemInfo(&ni);
    switch (ni.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64: m_architecture = L"x64";   break;
    case PROCESSOR_ARCHITECTURE_ARM64: m_architecture = L"ARM64"; break;
    case PROCESSOR_ARCHITECTURE_ARM:   m_architecture = L"ARM";   break;
    case PROCESSOR_ARCHITECTURE_INTEL: m_architecture = L"x86";   break;
    default:                           m_architecture = L"Unknown"; break;
    }

    // Physical cores + L1/L2/L3 cache — one variable-length pass over RelationAll.
    // Each cache entry is one physical cache instance (shared caches appear once),
    // so summing CacheSize per level gives the true total per level.
    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationAll, nullptr, &len);
    if (len == 0) return;

    std::vector<BYTE> buf(len);
    if (!GetLogicalProcessorInformationEx(RelationAll,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buf.data()), &len))
        return;

    BYTE* p   = buf.data();
    BYTE* end = p + len;
    while (p < end)
    {
        auto* cur = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(p);
        if (cur->Relationship == RelationProcessorCore)
        {
            ++m_physicalCores;
        }
        else if (cur->Relationship == RelationCache)
        {
            switch (cur->Cache.Level)
            {
            case 1: m_cacheL1 += cur->Cache.CacheSize; break;
            case 2: m_cacheL2 += cur->Cache.CacheSize; break;
            case 3: m_cacheL3 += cur->Cache.CacheSize; break;
            }
        }
        p += cur->Size;
    }
}

// CurrentVoltage isn't available through any other user-mode API — WMI is the
// only no-driver source, so this pays the one-time COM cost at startup only.
void WinApiCpuProvider::ReadVoltage()
{
    bool ownsCom = false;
    IWbemServices* svc = Wmi::Connect(L"ROOT\\CIMV2", ownsCom);
    if (svc)
    {
        if (IWbemClassObject* obj = Wmi::QueryFirst(svc, L"SELECT CurrentVoltage FROM Win32_Processor"))
        {
            double raw = 0.0;
            if (Wmi::GetU32(obj, L"CurrentVoltage", raw))
            {
                // High bit set = valid reading, low 7 bits = voltage in tenths of a volt.
                // High bit clear = legacy enumerated value (not an actual reading) — unsupported.
                long v = static_cast<long>(raw);
                if (v & 0x80)
                    m_voltage = (v & 0x7F) / 10.0;
            }
            obj->Release();
        }
        svc->Release();
    }
    if (ownsCom) CoUninitialize();
}
