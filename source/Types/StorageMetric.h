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
    VolumeLabel,
    FileSystem,
    DriveType,
    Unknown
};
