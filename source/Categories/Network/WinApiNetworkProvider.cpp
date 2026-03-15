#include "Categories/Network/WinApiNetworkProvider.h"
#include "Utils/Debug.h"

#include <iphlpapi.h>

bool WinApiNetworkProvider::Initialize()
{
    PMIB_IF_TABLE2 table = nullptr;

    if (GetIfTable2(&table) != NO_ERROR)
    {
        LOG_ERROR(L"GetIfTable2 failed");
        return false;
    }

    for (ULONG i = 0; i < table->NumEntries; i++)
    {
        auto& row = table->Table[i];

        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;

        if (row.Type == IF_TYPE_TUNNEL)
            continue;

        NetworkDevice device;

        device.luid = row.InterfaceLuid;
        device.prevRx = row.InOctets;
        device.prevTx = row.OutOctets;

        device.rxTotal = row.InOctets;
        device.txTotal = row.OutOctets;

        device.linkSpeed = row.TransmitLinkSpeed;

        m_devices.push_back(device);
    }

    FreeMibTable(table);

    m_prevTime = GetTickCount64();

    LOG_INFO(L"WinApiNetworkProvider initialized (%u adapters)", m_devices.size());

    return true;
}

void WinApiNetworkProvider::Update()
{
    PMIB_IF_TABLE2 table = nullptr;

    if (GetIfTable2(&table) != NO_ERROR)
    {
        LOG_ERROR(L"GetIfTable2 update failed");
        return;
    }

    ULONGLONG now = GetTickCount64();
    double deltaTime = (now - m_prevTime) / 1000.0;

    if (deltaTime <= 0)
        deltaTime = 1;

    for (auto& device : m_devices)
    {
        for (ULONG i = 0; i < table->NumEntries; i++)
        {
            auto& row = table->Table[i];

            if (row.InterfaceLuid.Value != device.luid.Value)
                continue;

            uint64_t rx = row.InOctets;
            uint64_t tx = row.OutOctets;

            uint64_t rxDelta = rx - device.prevRx;
            uint64_t txDelta = tx - device.prevTx;

            device.rxSpeed = rxDelta / deltaTime;
            device.txSpeed = txDelta / deltaTime;

            device.prevRx = rx;
            device.prevTx = tx;

            device.rxTotal = rx;
            device.txTotal = tx;

            device.linkSpeed = row.TransmitLinkSpeed;

            break;
        }
    }

    m_prevTime = now;

    FreeMibTable(table);
}

bool WinApiNetworkProvider::GetDownload(uint32_t deviceIndex, double& value)
{
    if (deviceIndex >= m_devices.size())
        return false;

    value = m_devices[deviceIndex].rxSpeed;

    return true;
}

bool WinApiNetworkProvider::GetUpload(uint32_t deviceIndex, double& value)
{
    if (deviceIndex >= m_devices.size())
        return false;

    value = m_devices[deviceIndex].txSpeed;

    return true;
}

bool WinApiNetworkProvider::GetDownloadTotal(uint32_t deviceIndex, double& value)
{
    if (deviceIndex >= m_devices.size())
        return false;

    value = static_cast<double>(m_devices[deviceIndex].rxTotal);

    return true;
}

bool WinApiNetworkProvider::GetUploadTotal(uint32_t deviceIndex, double& value)
{
    if (deviceIndex >= m_devices.size())
        return false;

    value = static_cast<double>(m_devices[deviceIndex].txTotal);

    return true;
}

bool WinApiNetworkProvider::GetSpeed(uint32_t deviceIndex, double& value)
{
    if (deviceIndex >= m_devices.size())
        return false;

    value = static_cast<double>(m_devices[deviceIndex].linkSpeed);

    return true;
}

uint32_t WinApiNetworkProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}