#pragma once

#include <cstdint>

enum class PingMetric : uint32_t
{
    Rtt,
    PacketLoss,
    MinRtt,
    MaxRtt,
    AvgRtt,
    Jitter,
    Ttl,
    PacketsSent,
    PacketsReceived,
    ResolvedIp, // string
    Status,     // string
    Unknown
};
