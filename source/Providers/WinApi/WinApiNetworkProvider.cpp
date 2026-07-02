#include "Providers/WinApi/WinApiNetworkProvider.h"
#include "Types/NetworkMetric.h"
#include "Utils/Debug.h"

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

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
        dev.luid      = row.InterfaceLuid;
        dev.prevRx    = row.InOctets;
        dev.prevTx    = row.OutOctets;
        dev.rxTotal   = row.InOctets;
        dev.txTotal   = row.OutOctets;
        dev.linkSpeed = row.TransmitLinkSpeed;
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

        uint64_t rxDelta = row.InOctets  - dev.prevRx;
        uint64_t txDelta = row.OutOctets - dev.prevTx;

        dev.rxSpeed   = rxDelta / m_deltaTime;
        dev.txSpeed   = txDelta / m_deltaTime;
        dev.prevRx    = row.InOctets;
        dev.prevTx    = row.OutOctets;
        dev.rxTotal   = row.InOctets;
        dev.txTotal   = row.OutOctets;
        dev.linkSpeed = row.TransmitLinkSpeed;
        break;
    }

    FreeMibTable(table);

    snap.Set(static_cast<uint32_t>(NetworkMetric::Download),      dev.rxSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::Upload),        dev.txSpeed);
    snap.Set(static_cast<uint32_t>(NetworkMetric::DownloadTotal), static_cast<double>(dev.rxTotal));
    snap.Set(static_cast<uint32_t>(NetworkMetric::UploadTotal),   static_cast<double>(dev.txTotal));
    snap.Set(static_cast<uint32_t>(NetworkMetric::Speed),         static_cast<double>(dev.linkSpeed));
}

bool WinApiNetworkProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return false;
}
