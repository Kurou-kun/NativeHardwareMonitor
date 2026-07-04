#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <wbemidl.h>

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

    void InitWmi();
    bool QueryPageFileUsage(double& usedBytes, double& totalBytes);
};
