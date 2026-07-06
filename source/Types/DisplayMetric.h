#pragma once

#include <cstdint>

// Multi-device: one device per active monitor (Device=0,1,...). Count is the
// same total on every device, for convenience.
enum class DisplayMetric : uint32_t
{
    Width,        // px, current mode
    Height,       // px
    RefreshRate,  // Hz
    BitsPerPixel, // bpp
    Primary,      // 1/0 — is the primary monitor
    Count,        // total active monitors (same value on every device)
    Name,         // string, friendly monitor name (e.g. "Generic PnP Monitor")
    DeviceName,   // string, GDI device (e.g. "\\.\DISPLAY1")
    Resolution,   // string, "WIDTHxHEIGHT"
    Unknown
};
