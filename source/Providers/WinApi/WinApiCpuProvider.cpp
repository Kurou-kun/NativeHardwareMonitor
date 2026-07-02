#include "Providers/WinApi/WinApiCpuProvider.h"
#include "Types/CpuMetric.h"
#include "Utils/Debug.h"

#include <algorithm>
#include <pdh.h>

#pragma comment(lib, "pdh.lib")

#define SystemProcessorPerformanceInformation 8

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

    QueryProcessorTimes(m_prevTimes);

    // Base clock from registry
    HKEY key;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &key) == ERROR_SUCCESS)
    {
        DWORD mhz = 0, size = sizeof(DWORD);
        if (RegQueryValueExW(key, L"~MHz", nullptr, nullptr, (LPBYTE)&mhz, &size) == ERROR_SUCCESS)
            m_baseClock = mhz * 1000000.0;
        RegCloseKey(key);
    }

    // PDH counters for clock
    if (PdhOpenQueryW(nullptr, 0, &m_query) == ERROR_SUCCESS)
    {
        PdhAddEnglishCounterW(m_query, L"\\Processor Information(_Total)\\Processor Frequency",       0, &m_freqCounter);
        PdhAddEnglishCounterW(m_query, L"\\Processor Information(_Total)\\% Processor Performance",   0, &m_perfCounter);
        PdhCollectQueryData(m_query);
    }

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

        // Clock
        if (m_query && m_freqCounter && m_perfCounter)
        {
            PdhCollectQueryData(m_query);

            PDH_FMT_COUNTERVALUE freq{}, perf{};
            double clockFreq = 0.0, clockPerf = 0.0;

            if (PdhGetFormattedCounterValue(m_freqCounter, PDH_FMT_DOUBLE, nullptr, &freq) == ERROR_SUCCESS)
                clockFreq = freq.doubleValue * 1000000.0;

            if (PdhGetFormattedCounterValue(m_perfCounter, PDH_FMT_DOUBLE, nullptr, &perf) == ERROR_SUCCESS)
                clockPerf = m_baseClock * std::min(perf.doubleValue, 120.0) / 100.0;

            m_currentClock = std::max(clockFreq, clockPerf);
        }

        snap.Set(static_cast<uint32_t>(CpuMetric::Usage), m_totalUsage);
        snap.Set(static_cast<uint32_t>(CpuMetric::Clock), m_currentClock);
    }
    else
    {
        uint32_t coreIdx = deviceIndex - 1;
        if (coreIdx < m_coreUsage.size())
            snap.Set(static_cast<uint32_t>(CpuMetric::Usage), m_coreUsage[coreIdx]);
    }
}

bool WinApiCpuProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return false;
}

bool WinApiCpuProvider::QueryProcessorTimes(std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION>& data)
{
    if (!m_ntQuery) return false;

    ULONG size = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * m_coreCount;
    return m_ntQuery(SystemProcessorPerformanceInformation, data.data(), size, nullptr) == 0;
}
