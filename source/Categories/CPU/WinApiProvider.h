#pragma once

#include "Categories/CPU/ICpuProvider.h"

#include <vector>
#include <Windows.h>
#include <PowrProf.h>

#pragma comment(lib, "PowrProf.lib")

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

//
// CPU usage structure used by NtQuerySystemInformation
//
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;

} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

//
// CPU clock structure used by CallNtPowerInformation
//
typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;

} PROCESSOR_POWER_INFORMATION;

class WinApiProvider : public ICpuProvider
{
public:

    bool Initialize() override;
    void Update() override;

    bool GetTotalUsage(double& value) override;
    bool GetCoreUsage(uint32_t coreIndex, double& value) override;

    bool GetClock(double& value) override;
    bool GetCoreClock(uint32_t coreIndex, double& value) override;

    uint32_t GetCoreCount() const override;

    bool GetTemperature(double& value) override;

private:

    uint32_t m_coreCount = 0;

    //
    // CPU usage snapshots
    //
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_prev;
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_curr;

    std::vector<double> m_coreUsage;
    double m_totalUsage = 0.0;

    //
    // CPU clock info
    //
    std::vector<PROCESSOR_POWER_INFORMATION> m_powerInfo;
    std::vector<double> m_coreClock;

    double m_totalClock = 0.0;
};