#pragma once

enum class StorageMetric : uint32_t
{
    ReadBytes,
    WriteBytes,
    ReadSpeed,
    WriteSpeed,
    UsedSpace,
    FreeSpace,
    TotalSpace,
    QueueLength,
    BusyPercent,
    ReadsPerSec,
    WritesPerSec,
    Temperature,
    VolumeLabel,
    FileSystem,
    DriveType,
    Unknown
};
