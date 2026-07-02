#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <winternl.h>
#include <pdh.h>
#include <vector>

typedef NTSTATUS(NTAPI* NtQuerySystemInformation_t)(int, void*, unsigned long, unsigned long*);

class WinApiCpuProvider : public IProvider
{
public:
    ~WinApiCpuProvider() { if (m_query) PdhCloseQuery(m_query); }

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    uint32_t m_coreCount = 0;
    double   m_totalUsage  = 0.0;
    double   m_currentClock = 0.0;
    double   m_baseClock    = 0.0;

    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_prevTimes;
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_currTimes;
    std::vector<double> m_coreUsage;

    NtQuerySystemInformation_t m_ntQuery = nullptr;

    PDH_HQUERY   m_query       = nullptr;
    PDH_HCOUNTER m_freqCounter = nullptr;
    PDH_HCOUNTER m_perfCounter = nullptr;

    bool QueryProcessorTimes(std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION>& data);
};
