#include "Categories/CPU/CpuBackend.h"
#include "Types/CpuMetric.h"

bool CpuBackend::OnInitialize()
{
    return ReadSnapshot(m_prev);
}

void CpuBackend::OnUpdate()
{
    if (!ReadSnapshot(m_curr))
        return;

    ULONGLONG idleDiff = m_curr.idle - m_prev.idle;
    ULONGLONG kernelDiff = m_curr.kernel - m_prev.kernel;
    ULONGLONG userDiff = m_curr.user - m_prev.user;

    ULONGLONG total = kernelDiff + userDiff;

    if (total > 0)
    {
        m_usagePercent =
            (1.0 - (double)idleDiff / total) * 100.0;
    }

    m_prev = m_curr;
}

double CpuBackend::GetValue(uint32_t, uint32_t metricId)
{
    if (metricId == static_cast<uint32_t>(CpuMetric::UsagePercent))
        return m_usagePercent;

    return 0.0;
}

bool CpuBackend::ReadSnapshot(Snapshot& snap)
{
    FILETIME idleTime, kernelTime, userTime;

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return false;

    snap.idle =
        ((ULONGLONG)idleTime.dwHighDateTime << 32) |
        idleTime.dwLowDateTime;

    snap.kernel =
        ((ULONGLONG)kernelTime.dwHighDateTime << 32) |
        kernelTime.dwLowDateTime;

    snap.user =
        ((ULONGLONG)userTime.dwHighDateTime << 32) |
        userTime.dwLowDateTime;

    return true;
}