#include "Providers/WinApi/WinApiDisplayProvider.h"
#include "Types/DisplayMetric.h"
#include "Utils/Debug.h"

bool WinApiDisplayProvider::Initialize()
{
    DISPLAY_DEVICEW adapter{};
    adapter.cb = sizeof(adapter);

    for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &adapter, 0); ++i)
    {
        // Skip adapters with no attached, active monitor (mirrors/off/pseudo).
        if (!(adapter.StateFlags & DISPLAY_DEVICE_ACTIVE))
            continue;

        Device dev;
        dev.deviceName = adapter.DeviceName; // "\\.\DISPLAYn"
        dev.primary    = (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;

        // Friendly monitor name comes from the monitor device under this adapter.
        DISPLAY_DEVICEW monitor{};
        monitor.cb = sizeof(monitor);
        if (EnumDisplayDevicesW(adapter.DeviceName, 0, &monitor, 0))
            dev.name = monitor.DeviceString; // e.g. "Generic PnP Monitor"

        m_devices.push_back(std::move(dev));
    }

    LOG_STARTUP(L"WinApiDisplayProvider: initialized");
    return true;
}

uint32_t WinApiDisplayProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}

void WinApiDisplayProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex >= m_devices.size())
        return;

    Device& dev = m_devices[deviceIndex];

    snap.Set(static_cast<uint32_t>(DisplayMetric::Count),   static_cast<double>(m_devices.size()));
    snap.Set(static_cast<uint32_t>(DisplayMetric::Primary), dev.primary ? 1.0 : 0.0);

    DEVMODEW dm{};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(dev.deviceName.c_str(), ENUM_CURRENT_SETTINGS, &dm))
    {
        snap.Set(static_cast<uint32_t>(DisplayMetric::Width),        static_cast<double>(dm.dmPelsWidth));
        snap.Set(static_cast<uint32_t>(DisplayMetric::Height),       static_cast<double>(dm.dmPelsHeight));
        snap.Set(static_cast<uint32_t>(DisplayMetric::RefreshRate),  static_cast<double>(dm.dmDisplayFrequency));
        snap.Set(static_cast<uint32_t>(DisplayMetric::BitsPerPixel), static_cast<double>(dm.dmBitsPerPel));

        wchar_t buf[32];
        swprintf(buf, 32, L"%lux%lu", dm.dmPelsWidth, dm.dmPelsHeight);
        dev.resolution = buf;
    }
}

bool WinApiDisplayProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_devices.size())
        return false;

    const Device& dev = m_devices[deviceIndex];

    switch (static_cast<DisplayMetric>(metricId))
    {
    case DisplayMetric::Name:       if (dev.name.empty())       return false; out = dev.name;       return true;
    case DisplayMetric::DeviceName: if (dev.deviceName.empty()) return false; out = dev.deviceName; return true;
    case DisplayMetric::Resolution: if (dev.resolution.empty()) return false; out = dev.resolution; return true;
    default:                        return false;
    }
}
