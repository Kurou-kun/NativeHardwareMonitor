#include "Providers/WinApi/WinApiNetworkProvider.h"
#include "Types/NetworkMetric.h"
#include "Utils/Debug.h"

#include <cstdio>
#include <iphlpapi.h>
#include <wlanapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wlanapi.lib")

static std::wstring PhyTypeName(DOT11_PHY_TYPE phy)
{
    switch (phy)
    {
    case dot11_phy_type_hrdsss: return L"802.11b";
    case dot11_phy_type_ofdm:   return L"802.11a";
    case dot11_phy_type_erp:    return L"802.11g";
    case dot11_phy_type_ht:     return L"802.11n";
    case dot11_phy_type_vht:    return L"802.11ac";
    case dot11_phy_type_he:     return L"802.11ax";  // Wi-Fi 6
    case dot11_phy_type_eht:    return L"802.11be";  // Wi-Fi 7
    default:                    return L"";
    }
}

static std::wstring SsidToWide(const DOT11_SSID& ssid)
{
    if (ssid.uSSIDLength == 0)
        return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0,
        reinterpret_cast<const char*>(ssid.ucSSID), static_cast<int>(ssid.uSSIDLength),
        nullptr, 0);
    if (len <= 0)
        return L"";

    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0,
        reinterpret_cast<const char*>(ssid.ucSSID), static_cast<int>(ssid.uSSIDLength),
        out.data(), len);
    return out;
}

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

WinApiNetworkProvider::~WinApiNetworkProvider()
{
    if (m_wlanHandle)
        WlanCloseHandle(m_wlanHandle, nullptr);
}

void WinApiNetworkProvider::UpdateWifi(Device& dev)
{
    dev.wifiSignal = -1.0;
    dev.wifiRxRate = -1.0;
    dev.wifiTxRate = -1.0;
    dev.ssid.clear();
    dev.wifiRadioType.clear();

    if (!m_wlanHandle)
        return;

    PWLAN_CONNECTION_ATTRIBUTES info = nullptr;
    DWORD size = 0;
    // Non-Wi-Fi GUIDs return ERROR_NOT_FOUND etc — just leave the fields at -1.
    DWORD res = WlanQueryInterface(m_wlanHandle, &dev.guid,
        wlan_intf_opcode_current_connection, nullptr, &size,
        reinterpret_cast<PVOID*>(&info), nullptr);

    if (res == ERROR_SUCCESS && info)
    {
        if (info->isState == wlan_interface_state_connected)
        {
            const auto& a = info->wlanAssociationAttributes;
            dev.wifiSignal    = static_cast<double>(a.wlanSignalQuality); // 0-100
            dev.wifiRxRate    = a.ulRxRate / 1000.0;  // kbps -> Mbps
            dev.wifiTxRate    = a.ulTxRate / 1000.0;
            dev.ssid          = SsidToWide(a.dot11Ssid);
            dev.wifiRadioType = PhyTypeName(a.dot11PhyType);
        }
        WlanFreeMemory(info);
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

    // Open a WLAN handle for Wi-Fi metrics; absence (no wireless service) is fine.
    DWORD negotiated = 0;
    if (WlanOpenHandle(WLAN_API_VERSION_2_0, nullptr, &negotiated, &m_wlanHandle) != ERROR_SUCCESS)
        m_wlanHandle = nullptr;

    for (ULONG i = 0; i < table->NumEntries; ++i)
    {
        auto& row = table->Table[i];
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK || row.Type == IF_TYPE_TUNNEL)
            continue;

        Device dev;
        dev.luid             = row.InterfaceLuid;
        dev.guid             = row.InterfaceGuid;
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

    UpdateWifi(dev);

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

    // Wi-Fi only — leave unset (reads -1) on wired/disconnected adapters.
    if (dev.wifiSignal >= 0.0)
    {
        snap.Set(static_cast<uint32_t>(NetworkMetric::WifiSignal), dev.wifiSignal);
        snap.Set(static_cast<uint32_t>(NetworkMetric::WifiRxRate), dev.wifiRxRate);
        snap.Set(static_cast<uint32_t>(NetworkMetric::WifiTxRate), dev.wifiTxRate);
    }
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
    case NetworkMetric::Ssid:             if (dev.ssid.empty())             return false; out = dev.ssid;             return true;
    case NetworkMetric::WifiRadioType:    if (dev.wifiRadioType.empty())    return false; out = dev.wifiRadioType;    return true;
    default:                              return false;
    }
}
