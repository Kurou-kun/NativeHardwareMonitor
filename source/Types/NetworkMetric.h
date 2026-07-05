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
    WifiSignal,      // % (0-100), wireless adapters only
    WifiRxRate,      // Mbps
    WifiTxRate,      // Mbps
    Ssid,            // string
    WifiRadioType,   // string, e.g. "802.11ax"
    Unknown
};
