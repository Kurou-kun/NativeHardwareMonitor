#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <wbemidl.h>
#include <string>

// Single "battery" device (index 0), always present even on desktops with no
// battery — the metrics simply report unsupported. Numeric live readings come
// from Win32 GetSystemPowerStatus (driverless, always available); the richer
// capacity/voltage/identity metrics come from the ROOT\WMI battery classes,
// which every ACPI-battery driver exposes without any custom kernel driver.
class WinApiBatteryProvider : public IProvider
{
public:
    ~WinApiBatteryProvider();

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    IWbemServices* m_wmiServices    = nullptr; // ROOT\WMI, kept alive across gathers
    bool           m_comInitialized = false;

    // Static battery identity (read once from BatteryStaticData at Initialize).
    double       m_designCapacity = 0.0; // mWh
    std::wstring m_chemistry;   // "LION", "NiMH", ...
    std::wstring m_manufacturer;
    std::wstring m_serialNumber;
    std::wstring m_deviceName;

    // Derived status string, refreshed each gather (read by GetString).
    std::wstring m_statusText;

    void ReadStaticData();   // BatteryStaticData — one query at startup
    void GatherWmi(Snapshot& snap); // BatteryStatus / FullChargedCapacity / CycleCount
};
