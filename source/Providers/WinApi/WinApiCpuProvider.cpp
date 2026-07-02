#include "Providers/WinApi/WinApiCpuProvider.h"
#include "Types/CpuMetric.h"
#include "Utils/Debug.h"

#include <algorithm>
#include <powrprof.h>
#include <powerbase.h>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "wbemuuid.lib")

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

    ReadName();
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
    if (metricId == static_cast<uint32_t>(CpuMetric::Name) && !m_name.empty())
    {
        out = m_name;
        return true;
    }
    return false;
}

bool WinApiCpuProvider::QueryProcessorTimes(std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION>& data)
{
    if (!m_ntQuery) return false;

    ULONG size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * m_coreCount;
    return m_ntQuery(SystemProcessorPerformanceInformation, data.data(), size, nullptr) == 0;
}

void WinApiCpuProvider::ReadName()
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &key) != ERROR_SUCCESS)
        return;

    wchar_t buf[256] = {};
    DWORD size = sizeof(buf);
    if (RegQueryValueExW(key, L"ProcessorNameString", nullptr, nullptr, (LPBYTE)buf, &size) == ERROR_SUCCESS)
    {
        m_name = buf;

        // Registry value is typically padded with spaces
        size_t start = m_name.find_first_not_of(L' ');
        size_t end   = m_name.find_last_not_of(L' ');
        m_name = (start == std::wstring::npos) ? L"" : m_name.substr(start, end - start + 1);
    }
    RegCloseKey(key);
}

// CurrentVoltage isn't available through any other user-mode API — WMI is the
// only no-driver source, so this pays the one-time COM cost at startup only.
void WinApiCpuProvider::ReadVoltage()
{
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (coInit == RPC_E_CHANGED_MODE)
        return; // already initialized with an incompatible model — skip rather than fight it

    IWbemLocator* locator = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                 IID_IWbemLocator, (LPVOID*)&locator)) || !locator)
    {
        CoUninitialize();
        return;
    }

    IWbemServices* services = nullptr;
    HRESULT hr = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr,
                                         0, nullptr, nullptr, &services);
    if (FAILED(hr) || !services)
    {
        locator->Release();
        CoUninitialize();
        return;
    }

    CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                       RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    IEnumWbemClassObject* enumerator = nullptr;
    hr = services->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT CurrentVoltage FROM Win32_Processor"),
                              WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);

    if (SUCCEEDED(hr) && enumerator)
    {
        IWbemClassObject* obj = nullptr;
        ULONG returned = 0;
        if (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK && obj)
        {
            VARIANT val;
            VariantInit(&val);
            if (SUCCEEDED(obj->Get(L"CurrentVoltage", 0, &val, nullptr, nullptr)) && val.vt == VT_I4)
            {
                // High bit set = valid reading, low 7 bits = voltage in tenths of a volt.
                // High bit clear = legacy enumerated value (not an actual reading) — unsupported.
                if (val.lVal & 0x80)
                    m_voltage = (val.lVal & 0x7F) / 10.0;
            }
            VariantClear(&val);
            obj->Release();
        }
        enumerator->Release();
    }

    services->Release();
    locator->Release();
    CoUninitialize();
}
