#include "Categories/CPU/WinApiProvider.h"

typedef NTSTATUS(NTAPI* NtQuerySystemInformation_t)(
    ULONG,
    PVOID,
    ULONG,
    PULONG
    );

static NtQuerySystemInformation_t NtQuerySystemInformationPtr = nullptr;

static const ULONG SystemProcessorPerformanceInformation = 8;

bool WinApiProvider::Initialize()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");

    if (!ntdll)
        return false;

    NtQuerySystemInformationPtr =
        (NtQuerySystemInformation_t)GetProcAddress(
            ntdll,
            "NtQuerySystemInformation");

    if (!NtQuerySystemInformationPtr)
        return false;

    m_coreCount = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);

    m_prev.resize(m_coreCount);
    m_curr.resize(m_coreCount);
    m_coreUsage.resize(m_coreCount);

    m_powerInfo.resize(m_coreCount);
    m_coreClock.resize(m_coreCount);

    ULONG size =
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        m_coreCount;

    if (NtQuerySystemInformationPtr(
        SystemProcessorPerformanceInformation,
        m_prev.data(),
        size,
        nullptr) != 0)
    {
        return false;
    }

    return true;
}

void WinApiProvider::Update()
{
    ULONG size =
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        m_coreCount;

    if (NtQuerySystemInformationPtr(
        SystemProcessorPerformanceInformation,
        m_curr.data(),
        size,
        nullptr) != 0)
    {
        return;
    }

    double totalIdle = 0;
    double totalTime = 0;

    for (uint32_t i = 0; i < m_coreCount; i++)
    {
        auto& prev = m_prev[i];
        auto& curr = m_curr[i];

        ULONGLONG idleDiff =
            curr.IdleTime.QuadPart -
            prev.IdleTime.QuadPart;

        ULONGLONG kernelDiff =
            curr.KernelTime.QuadPart -
            prev.KernelTime.QuadPart;

        ULONGLONG userDiff =
            curr.UserTime.QuadPart -
            prev.UserTime.QuadPart;

        ULONGLONG kernelWork =
            kernelDiff - idleDiff;

        ULONGLONG active =
            kernelWork + userDiff;

        ULONGLONG total =
            active + idleDiff;

        if (total > 0)
        {
            m_coreUsage[i] =
                (double)active /
                (double)total * 100.0;
        }

        totalIdle += idleDiff;
        totalTime += total;
    }

    if (totalTime > 0)
    {
        m_totalUsage =
            (double)(totalTime - totalIdle) /
            (double)totalTime * 100.0;
    }

    m_prev = m_curr;

    //
    // CPU CLOCK
    //

    ULONG psize =
        sizeof(PROCESSOR_POWER_INFORMATION) *
        m_coreCount;

    if (CallNtPowerInformation(
        ProcessorInformation,
        nullptr,
        0,
        m_powerInfo.data(),
        psize) == 0)
    {
        double totalClock = 0;

        for (uint32_t i = 0; i < m_coreCount; i++)
        {
            double hz = (double)m_powerInfo[i].CurrentMhz * 1000000.0;

            if (hz <= 0)
                hz = (double)m_powerInfo[i].MaxMhz * 1000000.0;

            m_coreClock[i] = hz;

            totalClock += hz;
        }


        if (m_coreCount > 0)
        {
            m_totalClock =
                totalClock / (double)m_coreCount;
        }
    }
}

bool WinApiProvider::GetTotalUsage(double& value)
{
    value = m_totalUsage;
    return true;
}

bool WinApiProvider::GetCoreUsage(uint32_t coreIndex, double& value)
{
    if (coreIndex >= m_coreCount)
        return false;

    value = m_coreUsage[coreIndex];
    return true;
}

bool WinApiProvider::GetClock(double& value)
{
    value = m_totalClock;
    return true;
}

bool WinApiProvider::GetCoreClock(uint32_t coreIndex, double& value)
{
    if (coreIndex >= m_coreCount)
        return false;

    value = m_coreClock[coreIndex];
    return true;
}

uint32_t WinApiProvider::GetCoreCount() const
{
    return m_coreCount;
}

bool WinApiProvider::GetTemperature(double&)
{
    return false;
}