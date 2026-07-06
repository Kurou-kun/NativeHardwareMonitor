#pragma once

#include <cstdint>

enum class SystemMetric : uint32_t
{
    Uptime,       // seconds since boot
    ProcessCount, // count
    ThreadCount,  // count
    HandleCount,  // count
    OsName,       // string, e.g. "Microsoft Windows 11 Pro"
    OsVersion,    // string, e.g. "10.0.26200"
    OsBuild,      // string, e.g. "26200"
    Hostname,     // string
    UserName,     // string
    BootTime,     // string, localized "YYYY-MM-DD HH:MM:SS"
    Unknown
};
