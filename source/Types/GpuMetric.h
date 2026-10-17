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

    // System memory this adapter has in use, from the same "GPU Adapter Memory"
    // counter Task Manager reads. No vendor SDK reports it: ADLX, NVML and IGCL all
    // answer for the card's own memory only. It matters on an integrated GPU, whose
    // dedicated carve-out is a fixed few hundred MB no matter how much it actually
    // uses - so VramUsed alone understates it, and a tool that adds the two pools
    // together reads higher than we do.
    VramShared,
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
