#include "Categories/Network/NetworkBackend.h"
#include "Types/NetworkMetric.h"

bool NetworkBackend::OnInitialize()
{
    m_prevTime = GetTickCount64();
    return ReadSnapshot(m_prev);
}

void NetworkBackend::OnUpdate()
{
    uint64_t now = GetTickCount64();
    uint64_t deltaMs = now - m_prevTime;

    if (deltaMs == 0)
        return;

    if (!ReadSnapshot(m_curr))
        return;

    uint64_t rxDiff = m_curr.rxBytes - m_prev.rxBytes;
    uint64_t txDiff = m_curr.txBytes - m_prev.txBytes;

    m_rxPerSec = (double)rxDiff * 1000.0 / deltaMs;
    m_txPerSec = (double)txDiff * 1000.0 / deltaMs;

    m_prev = m_curr;
    m_prevTime = now;
}

double NetworkBackend::GetValue(uint32_t, uint32_t metricId)
{
    if (metricId == static_cast<uint32_t>(NetworkMetric::RxBytesPerSec))
        return m_rxPerSec;

    if (metricId == static_cast<uint32_t>(NetworkMetric::TxBytesPerSec))
        return m_txPerSec;

    return 0.0;
}

bool NetworkBackend::ReadSnapshot(Snapshot& snap)
{
    PMIB_IF_TABLE2 table = nullptr;

    if (GetIfTable2(&table) != NO_ERROR)
        return false;

    uint64_t totalRx = 0;
    uint64_t totalTx = 0;

    for (ULONG i = 0; i < table->NumEntries; ++i)
    {
        const MIB_IF_ROW2& row = table->Table[i];

        if (row.OperStatus == IfOperStatusUp)
        {
            totalRx += row.InOctets;
            totalTx += row.OutOctets;
        }
    }

    FreeMibTable(table);

    snap.rxBytes = totalRx;
    snap.txBytes = totalTx;

    return true;
}