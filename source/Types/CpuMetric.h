#pragma once

enum class CpuMetric : uint32_t
{
    Usage,
    Clock,
    MaxClock,
    Voltage,
    Name,
    Vendor,
    Identifier,
    MicrocodeVersion,
    CoreCount,
    ThreadCount,
    CacheL1,
    CacheL2,
    CacheL3,
    Architecture,
    Unknown
};
