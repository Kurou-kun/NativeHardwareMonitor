#pragma once

#include <cstdint>

enum class BatteryMetric : uint32_t
{
    ChargeLevel,        // %
    Charging,           // 1/0
    AcOnline,           // 1/0
    TimeToEmpty,        // seconds (-1 when charging/unknown)
    Rate,               // mW, signed (+ charging, - discharging)
    Voltage,            // mV
    RemainingCapacity,  // mWh
    FullChargeCapacity, // mWh
    DesignCapacity,     // mWh
    WearLevel,          // % = 100 * (1 - Full/Design)
    CycleCount,         // count (often unsupported -> -1)
    Status,             // string: Charging / Discharging / Full / AC
    Chemistry,          // string, e.g. "LION"
    Manufacturer,       // string
    SerialNumber,       // string
    DeviceName,         // string
    Unknown
};
