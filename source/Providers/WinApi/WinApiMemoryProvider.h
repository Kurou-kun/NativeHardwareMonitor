#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <wbemidl.h>
#include <string>

class WinApiMemoryProvider : public IProvider
{
public:
    ~WinApiMemoryProvider();

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    MEMORYSTATUSEX m_status{};

    // WMI connection kept alive across gathers (unlike CPU's one-shot voltage read) —
    // Win32_PageFileUsage is the only source for the actual pagefile.sys size/usage
    // on disk, distinct from the Commit Charge GetPerformanceInfo gives the Virtual device.
    IWbemServices* m_wmiServices    = nullptr;
    bool           m_comInitialized = false;

    // Static RAM hardware identity, read once at Initialize via Win32_PhysicalMemory.
    double       m_ramSpeed    = 0.0; // configured (EXPO/XMP) MT/s, falls back to rated
    uint32_t     m_moduleCount = 0;
    std::wstring m_memoryType;   // DDR4 / DDR5 / ...
    std::wstring m_manufacturer;
    std::wstring m_partNumber;

    void InitWmi();
    void ReadPhysicalMemory(); // RAM identity — one WMI query at startup, reuses m_wmiServices
    bool QueryPageFileUsage(double& usedBytes, double& totalBytes);
};
