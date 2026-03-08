#pragma once

#include <windows.h>
#include <winternl.h>
#include <pdh.h>
#include <vector>

#include "Categories/CPU/ICpuProvider.h"

typedef NTSTATUS(NTAPI* NtQuerySystemInformation_t)(
    int SystemInformationClass,
    void* SystemInformation,
    unsigned long SystemInformationLength,
    unsigned long* ReturnLength
    );

class WinApiProvider : public ICpuProvider
{
public:

    bool Initialize() override;
    void Update() override;

    bool GetTotalUsage(double& value) override;
    bool GetCoreUsage(uint32_t coreIndex, double& value) override;

    bool GetClock(double& value) override;

    uint32_t GetCoreCount() const override;

private:

    bool QueryProcessorInfo(
        std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION>& data
    );

private:

    uint32_t m_coreCount = 0;

    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_prevTimes;
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_currTimes;

    std::vector<double> m_coreUsage;

    double m_totalUsage = 0.0;

    double m_baseClock = 0.0;
    double m_currentClock = 0.0;
    double m_maxClock = 0.0;

    NtQuerySystemInformation_t m_ntQuerySystemInformation = nullptr;

    PDH_HQUERY m_perfQuery = nullptr;
    PDH_HCOUNTER m_perfCounter = nullptr;
    PDH_HCOUNTER m_freqCounter = nullptr;
};