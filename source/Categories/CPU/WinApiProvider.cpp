#include <windows.h>
#include <winternl.h>
#include <pdh.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "pdh.lib")

#include "Categories/CPU/WinApiProvider.h"
#include "Utils/Debug.h"

#define SystemProcessorPerformanceInformation 8
#define STATUS_SUCCESS 0

bool WinApiProvider::Initialize()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");

    if (!ntdll)
        return false;

    m_ntQuerySystemInformation =
        (NtQuerySystemInformation_t)GetProcAddress(
            ntdll,
            "NtQuerySystemInformation"
        );

    if (!m_ntQuerySystemInformation)
        return false;

    SYSTEM_INFO info;
    GetSystemInfo(&info);

    m_coreCount = info.dwNumberOfProcessors;

    m_prevTimes.resize(m_coreCount);
    m_currTimes.resize(m_coreCount);
    m_coreUsage.resize(m_coreCount, 0.0);

    QueryProcessorInfo(m_prevTimes);

    /*
        Base clock from registry
    */

    HKEY key;

    if (RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0,
        KEY_READ,
        &key) == ERROR_SUCCESS)
    {
        DWORD mhz = 0;
        DWORD size = sizeof(DWORD);

        if (RegQueryValueExW(
            key,
            L"~MHz",
            nullptr,
            nullptr,
            (LPBYTE)&mhz,
            &size) == ERROR_SUCCESS)
        {
            m_baseClock = (double)mhz * 1000000.0;
        }

        RegCloseKey(key);
    }

    /*
        PDH performance counter for clock estimation
    */

    if (PdhOpenQueryW(nullptr, 0, &m_perfQuery) == ERROR_SUCCESS)
    {
        PdhAddEnglishCounterW(
            m_perfQuery,
            L"\\Processor Information(_Total)\\Processor Frequency",
            0,
            &m_freqCounter
        );

        PdhAddEnglishCounterW(
            m_perfQuery,
            L"\\Processor Information(_Total)\\% Processor Performance",
            0,
            &m_perfCounter
        );

        PdhCollectQueryData(m_perfQuery);
    }

    LOG_INFO(
        L"WinApiProvider initialized",
        m_coreCount,
        m_baseClock / 1000000.0
    );

    return true;
}

void WinApiProvider::Update()
{
    if (!QueryProcessorInfo(m_currTimes))
    {
        LOG_ERROR(L"WinApiProvider: QueryProcessorInfo failed");
        return;
    }

    double totalUsage = 0.0;

    for (uint32_t i = 0; i < m_coreCount; i++)
    {
        auto& prev = m_prevTimes[i];
        auto& curr = m_currTimes[i];

        long long idle =
            curr.IdleTime.QuadPart - prev.IdleTime.QuadPart;

        long long kernel =
            curr.KernelTime.QuadPart - prev.KernelTime.QuadPart;

        long long user =
            curr.UserTime.QuadPart - prev.UserTime.QuadPart;

        long long total = kernel + user;

        double usage = 0.0;

        if (total > 0)
            usage = (double)(total - idle) / (double)total * 100.0;

        m_coreUsage[i] = usage;
        totalUsage += usage;
    }

    m_totalUsage = totalUsage / m_coreCount;

    m_prevTimes = m_currTimes;

    /*
        Clock calculation
    */

    if (!m_perfQuery || !m_freqCounter || !m_perfCounter)
    {
        LOG_ERROR(L"WinApiProvider: PDH counters not initialized");
        return;
    }

    if (PdhCollectQueryData(m_perfQuery) != ERROR_SUCCESS)
    {
        LOG_ERROR(L"WinApiProvider: PdhCollectQueryData failed");
        return;
    }

    PDH_FMT_COUNTERVALUE freqValue;
    PDH_FMT_COUNTERVALUE perfValue;

    double clockFreq = 0.0;
    double clockPerf = 0.0;

    if (PdhGetFormattedCounterValue(
        m_freqCounter,
        PDH_FMT_DOUBLE,
        nullptr,
        &freqValue) != ERROR_SUCCESS)
    {
        LOG_ERROR(L"WinApiProvider: Failed reading Processor Frequency counter");
    }
    else
    {
        clockFreq = freqValue.doubleValue * 1000000.0;
    }

    if (PdhGetFormattedCounterValue(
        m_perfCounter,
        PDH_FMT_DOUBLE,
        nullptr,
        &perfValue) != ERROR_SUCCESS)
    {
        LOG_ERROR(L"WinApiProvider: Failed reading Processor Performance counter");
    }
    else
    {
        double perf = perfValue.doubleValue;

        if (perf > 120.0)
            perf = 120.0;

        clockPerf = (m_baseClock * perf) / 100.0;
    }

    m_currentClock = std::max(clockFreq, clockPerf);

    if (m_currentClock <= 0.0)
    {
        LOG_ERROR(L"WinApiProvider: Invalid clock value detected");
    }

    if (m_currentClock > m_maxClock)
        m_maxClock = m_currentClock;
}

bool WinApiProvider::GetTotalUsage(double& value)
{
    value = m_totalUsage;
    return true;
}

bool WinApiProvider::GetCoreUsage(uint32_t coreIndex, double& value)
{
    if (coreIndex >= m_coreUsage.size())
        return false;

    value = m_coreUsage[coreIndex];
    return true;
}

bool WinApiProvider::GetClock(double& value)
{
    value = m_currentClock;
    return true;
}

uint32_t WinApiProvider::GetCoreCount() const
{
    return m_coreCount;
}

bool WinApiProvider::QueryProcessorInfo(
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION>& data)
{
    if (!m_ntQuerySystemInformation)
        return false;

    ULONG size =
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * m_coreCount;

    NTSTATUS status =
        m_ntQuerySystemInformation(
            SystemProcessorPerformanceInformation,
            data.data(),
            size,
            nullptr
        );

    return status == STATUS_SUCCESS;
}