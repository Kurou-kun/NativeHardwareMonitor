#include "Providers/WinApi/WinApiNetworkProvider.h"
#include "Types/NetworkMetric.h"
#include "Utils/Debug.h"

#include <cstdio>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

static std::wstring FormatPhysicalAddress(const UCHAR* addr, ULONG length)
{
    if (length == 0)
        return L"";

    std::wstring out;
    wchar_t byteStr[4];
    for (ULONG i = 0; i < length; ++i)
    {
        swprintf_s(byteStr, L"%02X", addr[i]);
        if (i > 0) out += L":";
        out += byteStr;
    }
    return out;
}

static std::wstring FormatConnectionStatus(NET_IF_MEDIA_CONNECT_STATE state)
{
    switch (state)
    {
    case MediaConnectStateConnected:    return L"Connected";
    case MediaConnectStateDisconnected: return L"Disconnected";
    default:                            return L"Unknown";
    }
}

bool WinApiNetworkProvider::Initialize()
{
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR)
    {
        LOG_STARTUP(L"WinApiNetworkProvider: GetIfTable2 failed");
        return false;
    }

    for (ULONG i = 0; i < table->NumEntries; ++i)
    {
        auto& row = table->Table[i];
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK || row.Type == IF_TYPE_TUNNEL)
            continue;

        Device dev;
        dev.luid             = row.InterfaceLuid;
        dev.prevRx           = row.InOctets;
        dev.prevTx           = row.OutOctets;
        dev.rxTotal          = row.InOctets;
        dev.txTotal          = row.OutOctets;
        dev.linkSpeed        = row.TransmitLinkSpeed;
        dev.receiveLinkSpeed = row.ReceiveLinkSpeed;
        dev.prevPacketsRx    = row.InUcastPkts;
        dev.prevPacketsTx    = row.OutUcastPkts;
        dev.prevErrorsRx     = row.InErrors;
        dev.prevErrorsTx     = row.OutErrors;
        dev.prevDiscardsRx   = row.InDiscards;
        dev.prevDiscardsTx   = row.OutDiscards;
        dev.mtu              = row.Mtu;
        dev.alias            = row.Alias;
        dev.description      = row.Description;
        dev.physicalAddress  = FormatPhysicalAddress(row.PhysicalAddress, row.PhysicalAddressLength);
        dev.connectionStatus = FormatConnectionStatus(row.MediaConnectState);
        m_devices.push_back(dev);
    }

    FreeMibTable(table);
    m_prevTime = GetTickCount64();

    LOG_STARTUP(L"WinApiNetworkProvider: initialized (%u adapter(s))", (uint32_t)m_devices.size());
    return true;
}

uint32_t WinApiNetworkProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}

void WinApiNetworkProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex >= m_devices.size())
        return;

    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR)
        return;

    // Compute deltaTime once per cycle (device 0) so all adapters share the same interval
    if (deviceIndex == 0)
    {
        ULONGLONG now  = GetTickCount64();
        double    dt   = (now - m_prevTime) / 1000.0;
        m_deltaTime    = dt > 0.0 ? dt : 1.0;
        m_prevTime     = now;
    }

    auto& dev = m_devices[deviceIndex];

    for (ULONG i = 0; i < table->NumEntries; ++i)
    {
        auto& row = table->Table[i];
        if (row.InterfaceLuid.Value != dev.luid.Value) continue;

        uint64_t rxDelta       = row.InOctets    - dev.prevRx;
        uint64_t txDelta       = row.OutOctets   - dev.prevTx;
        uint64_t packetsRxDelta  = row.InUcastPkts - dev.prevPacketsRx;
        uint64_t packetsTxDelta  = row.OutUcastPkts - dev.prevPacketsTx;
        uint64_t errorsRxDelta   = row.InErrors     - dev.prevErrorsRx;
        uint64_t errorsTxDelta   = row.OutErrors    - dev.prevErrorsTx;
        uint64_t discardsRxDelta = row.InDiscards   - dev.prevDiscardsRx;
        uint64_t discardsTxDelta = row.OutDiscards  - dev.prevDiscardsTx;

        dev.rxSpeed          = rxDelta / m_deltaTime;
        dev.txSpeed          = txDelta / m_deltaTime;
        dev.packetsRxSpeed   = packetsRxDelta / m_deltaTime;
        dev.packetsTxSpeed   = packetsTxDelta / m_deltaTime;
        dev.errorsRxSpeed    = errorsRxDelta / m_deltaTime;
        dev.errorsTxSpeed    = errorsTxDelta / m_deltaTime;
        dev.discardsRxSpeed  = discardsRxDelta / m_deltaTime;
        dev.discardsTxSpeed  = discardsTxDelta / m_deltaTime;

        dev.prevRx           = row.InOctets;
        dev.prevTx           = row.OutOctets;
        dev.prevPacketsRx    = row.InUcastPkts;
        dev.prevPacketsTx    = row.OutUcastPkts;
        dev.prevErrorsRx     = row.InErrors;
        dev.prevErrorsTx     = row.OutErrors;
        dev.prevDiscardsRx   = row.InDiscards;
        dev.prevDiscardsTx   = row.OutDiscards;

        dev.rxTotal          = row.InOctets;
        dev.txTotal          = row.OutOctets;
        dev.linkSpeed        = row.TransmitLinkSpeed;
        dev.receiveLinkSpeed = row.ReceiveLinkSpeed;
        dev.connectionStatus = FormatConnectionStatus(row.MediaConnectState);
        break;
    }

    FreeMibTable(table);

    snap.Set(static_cast<uint32_t>(NetworkMetric::Download),         dev.rxSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::Upload),           dev.txSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::DownloadTotal),    static_cast<double>(dev.rxTotal));
    snap.Set(static_cast<uint32_t>(NetworkMetric::UploadTotal),      static_cast<double>(dev.txTotal));
    snap.Set(static_cast<uint32_t>(NetworkMetric::Speed),            static_cast<double>(dev.linkSpeed));
    snap.Set(static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed), static_cast<double>(dev.receiveLinkSpeed));
    snap.Set(static_cast<uint32_t>(NetworkMetric::PacketsReceived),  dev.packetsRxSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::PacketsSent),      dev.packetsTxSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::ErrorsReceived),   dev.errorsRxSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::ErrorsSent),       dev.errorsTxSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::DiscardsReceived), dev.discardsRxSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::DiscardsSent),     dev.discardsTxSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::Mtu),              static_cast<double>(dev.mtu));
}

bool WinApiNetworkProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_devices.size())
        return false;

    auto& dev = m_devices[deviceIndex];

    switch (static_cast<NetworkMetric>(metricId))
    {
    case NetworkMetric::Alias:            if (dev.alias.empty())            return false; out = dev.alias;            return true;
    case NetworkMetric::Description:      if (dev.description.empty())      return false; out = dev.description;      return true;
    case NetworkMetric::PhysicalAddress:  if (dev.physicalAddress.empty())  return false; out = dev.physicalAddress;  return true;
    case NetworkMetric::ConnectionStatus: if (dev.connectionStatus.empty()) return false; out = dev.connectionStatus; return true;
    default:                              return false;
    }
}
