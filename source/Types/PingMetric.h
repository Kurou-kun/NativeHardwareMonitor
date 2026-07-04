#pragma once

#include <cstdint>

enum class PingMetric : uint32_t
{
    Rtt,
    PacketLoss,
    Unknown
};
