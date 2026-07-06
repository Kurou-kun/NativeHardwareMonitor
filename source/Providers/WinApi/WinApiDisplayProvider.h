#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <vector>
#include <string>

// One device per active monitor. Enumerated once at Initialize (fixed count —
// hotplug isn't tracked); resolution/refresh are re-read live each gather so a
// resolution change is picked up. All via user32 (EnumDisplayDevices /
// EnumDisplaySettings) — driverless.
class WinApiDisplayProvider : public IProvider
{
public:
    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    struct Device
    {
        std::wstring deviceName;  // "\\.\DISPLAY1" (static)
        std::wstring name;        // friendly monitor name (static)
        bool         primary = false;
        std::wstring resolution;  // "WxH", refreshed each gather (read by GetString)
    };

    std::vector<Device> m_devices;
};
