#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <winternl.h>
#include <vector>
#include <string>

typedef NTSTATUS(NTAPI* NtQuerySystemInformation_t)(int, void*, unsigned long, unsigned long*);

class WinApiCpuProvider : public IProvider
{
public:
    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    uint32_t m_coreCount     = 0; // logical processors (threads)
    uint32_t m_physicalCores = 0; // filled by ReadTopology(); 0 if unavailable
    uint64_t m_cacheL1       = 0; // total bytes per level, summed across all cache instances
    uint64_t m_cacheL2       = 0;
    uint64_t m_cacheL3       = 0;
    double   m_totalUsage    = 0.0;
    double   m_totalClock    = 0.0;
    double   m_totalMaxClock = 0.0;
    double   m_voltage       = -1.0; // cached at Initialize(); stays -1 if WMI can't supply it
    bool     m_clockValid    = false;
    std::wstring m_name;
    std::wstring m_vendor;
    std::wstring m_identifier;
    std::wstring m_microcodeVersion;
    std::wstring m_architecture;

    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_prevTimes;
    std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> m_currTimes;
    std::vector<double> m_coreUsage;
    std::vector<double> m_coreClock;
    std::vector<double> m_coreMaxClock;

    NtQuerySystemInformation_t m_ntQuery = nullptr;

    bool QueryProcessorTimes(std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION>& data);
    void ReadIdentity(); // Name, Vendor, Identifier, MicrocodeVersion — one registry key, one open
    void ReadTopology(); // physical core count + L1/L2/L3 cache sizes + architecture, all at Initialize
    void ReadVoltage(); // one-shot WMI query — CurrentVoltage isn't exposed anywhere else without a driver
};
