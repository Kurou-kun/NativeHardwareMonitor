#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>

#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")

#include "Categories/Network/NetworkBackend.h"
#include "Types/NetworkMetric.h"

bool NetworkBackend::OnInitialize()
{
    m_prevTime = GetTickCount64();
    return ReadSnapshot();
}

void NetworkBackend::OnUpdate()
{
    uint64_t now = GetTickCount64();
    uint64_t deltaMs = now - m_prevTime;

    if (deltaMs == 0)
        return;

    if (!ReadSnapshot())
        return;

    for (auto& a : m_adapters)
    {
        uint64_t rxDiff = a.currRx - a.prevRx;
        uint64_t txDiff = a.currTx - a.prevTx;

        a.rxPerSec = (double)rxDiff * 1000.0 / deltaMs;
        a.txPerSec = (double)txDiff * 1000.0 / deltaMs;

        a.prevRx = a.currRx;
        a.prevTx = a.currTx;
    }

    m_prevTime = now;
}

double NetworkBackend::GetValue(uint32_t deviceIndex, uint32_t metricId)
{
    if (deviceIndex >= m_adapters.size())
        return 0.0;

    const Adapter& a = m_adapters[deviceIndex];

    if (metricId == static_cast<uint32_t>(NetworkMetric::RxBytesPerSec))
        return a.rxPerSec;

    if (metricId == static_cast<uint32_t>(NetworkMetric::TxBytesPerSec))
        return a.txPerSec;

    return 0.0;
}

uint32_t NetworkBackend::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_adapters.size());
}

const std::wstring& NetworkBackend::GetDeviceName(uint32_t index) const
{
    static std::wstring empty;

    if (index >= m_adapters.size())
        return empty;

    return m_adapters[index].name;
}

bool NetworkBackend::ReadSnapshot()
{
    PMIB_IFTABLE table = nullptr;

    DWORD size = 0;
    GetIfTable(nullptr, &size, FALSE);

    table = (PMIB_IFTABLE)malloc(size);
    if (!table)
        return false;

    if (GetIfTable(table, &size, FALSE) != NO_ERROR)
    {
        free(table);
        return false;
    }

    if (m_adapters.size() != table->dwNumEntries)
        m_adapters.resize(table->dwNumEntries);

    for (DWORD i = 0; i < table->dwNumEntries; ++i)
    {
        const MIB_IFROW& row = table->table[i];

        Adapter& adapter = m_adapters[i];

        adapter.name = std::wstring((wchar_t*)row.wszName);

        adapter.active = (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL);

        adapter.currRx = row.dwInOctets;
        adapter.currTx = row.dwOutOctets;

        if (adapter.prevRx == 0 && adapter.prevTx == 0)
        {
            adapter.prevRx = adapter.currRx;
            adapter.prevTx = adapter.currTx;
        }
    }

    free(table);

    std::sort(
        m_adapters.begin(),
        m_adapters.end(),
        [](const Adapter& a, const Adapter& b)
        {
            return a.active > b.active;
        });

    return true;
}