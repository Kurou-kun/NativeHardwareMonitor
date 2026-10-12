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

    // What NVML reports for the graphics clock. On a power-gated GPU that is an averaged
    // "effective" clock far below the P-state clock CoreClock carries - the two are the
    // same number on a card that is awake.
    EffectiveClock,

    // String metrics — fetched via GetString(), not GatherSnapshot()
    Name,
    DriverVersion,
    VbiosVersion,
    PciDeviceId,
    ThrottleReasons,

    Unknown
};
