#pragma once

enum class NetworkMetric : uint32_t
{
    Download,
    Upload,
    DownloadTotal,
    UploadTotal,
    Speed,
    ReceiveLinkSpeed,
    PacketsReceived,
    PacketsSent,
    ErrorsReceived,
    ErrorsSent,
    DiscardsReceived,
    DiscardsSent,
    Mtu,
    Alias,
    Description,
    PhysicalAddress,
    ConnectionStatus,
    Unknown
};
