#include "Categories/Memory/MemoryBackend.h"

bool MemoryBackend::OnInitialize()
{
    return true;
}

void MemoryBackend::OnUpdate()
{
    MEMORYSTATUSEX statex{};
    statex.dwLength = sizeof(statex);

    if (GlobalMemoryStatusEx(&statex))
    {
        m_snapshot.totalBytes = statex.ullTotalPhys;
        m_snapshot.availBytes = statex.ullAvailPhys;
    }
}

double MemoryBackend::GetValue(uint32_t deviceIndex, uint32_t metricId)
{
    (void)deviceIndex;

    const uint64_t total = m_snapshot.totalBytes;
    const uint64_t free = m_snapshot.availBytes;
    const uint64_t used = total - free;

    switch (metricId)
    {
    case 0: // Total MB
        return static_cast<double>(total) / (1024.0 * 1024.0);

    case 1: // Used MB
        return static_cast<double>(used) / (1024.0 * 1024.0);

    case 2: // Free MB
        return static_cast<double>(free) / (1024.0 * 1024.0);

    case 3: // Usage %
        if (total == 0)
            return 0.0;
        return (static_cast<double>(used) / total) * 100.0;

    default:
        return 0.0;
    }
}