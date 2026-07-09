#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <string>
#include <vector>
#include <igcl_api.h>

// Intel GPU telemetry via IGCL (Intel Graphics Control Library).
// Loads ControlLib.dll, which ships with the Intel graphics driver — driverless,
// no kernel component. Single API, no per-metric backup (see GpuResolver).
class IgclProvider : public IProvider
{
public:
    ~IgclProvider() { Shutdown(); }

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    struct Device
    {
        ctl_device_adapter_handle_t handle = nullptr;
        std::string name;           // cached at Initialize()
        uint32_t    pciDeviceId = 0;

        // Power/utilization arrive as monotonic counters; instantaneous values
        // need a per-tick delta against the previous sample. Seeded on first read.
        bool   seeded       = false;
        double prevTime     = 0.0;  // seconds
        double prevEnergy   = 0.0;  // joules
        double prevActivity = 0.0;  // seconds-busy
    };

    HMODULE                m_module = nullptr;
    ctl_api_handle_t       m_api    = nullptr;
    std::vector<Device>    m_devices;

    bool LoadFunctions();
    void Shutdown();

    decltype(&ctlInit)                m_Init          = nullptr;
    decltype(&ctlClose)               m_Close         = nullptr;
    decltype(&ctlEnumerateDevices)    m_Enumerate     = nullptr;
    decltype(&ctlGetDeviceProperties) m_GetProperties = nullptr;
    decltype(&ctlPowerTelemetryGet)   m_GetTelemetry  = nullptr;
};
