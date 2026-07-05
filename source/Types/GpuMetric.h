#pragma once

enum class GpuMetric : uint32_t
{
    Usage,
    CoreClock,
    MemoryClock,
    Temperature,
    HotspotTemperature,
    MemoryTemperature,
    VramUsed,
    VramTotal,
    FanSpeed,
    FanSpeedRPM,
    Power,
    PowerLimit,
    Voltage,
    IntakeTemperature,
    TotalBoardPower,
    MaxCoreClock,
    PcieLinkGen,
    PcieLinkWidth,
    EncoderUsage,
    DecoderUsage,
    PerfState,

    // String metrics — fetched via GetString(), not GatherSnapshot()
    Name,
    DriverVersion,
    VbiosVersion,
    PciDeviceId,
    ThrottleReasons,

    Unknown
};
