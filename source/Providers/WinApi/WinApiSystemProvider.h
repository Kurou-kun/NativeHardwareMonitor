#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <wbemidl.h>
#include <string>

// Single "system" device (index 0). Live OS/runtime counters come from Win32
// (GetTickCount64 / GetPerformanceInfo); OS identity strings from a one-shot
// WMI Win32_OperatingSystem query. All driverless.
class WinApiSystemProvider : public IProvider
{
public:
    ~WinApiSystemProvider();

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    IWbemServices* m_wmiServices    = nullptr; // ROOT\CIMV2
    bool           m_comInitialized = false;

    // Static strings, read once at Initialize.
    std::wstring m_osName;
    std::wstring m_osVersion;
    std::wstring m_osBuild;
    std::wstring m_hostname;
    std::wstring m_userName;
    std::wstring m_bootTime;

    void ReadOsInfo();   // Win32_OperatingSystem
    void ReadIdentity(); // hostname / username / boot time (Win32)
};
