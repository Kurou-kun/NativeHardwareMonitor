#include "Providers/WinApi/WinApiMemoryProvider.h"
#include "Types/MemoryMetric.h"
#include "Utils/Debug.h"

bool WinApiMemoryProvider::Initialize()
{
    m_status.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&m_status))
    {
        LOG_STARTUP(L"WinApiMemoryProvider: GlobalMemoryStatusEx failed");
        return false;
    }

    LOG_STARTUP(L"WinApiMemoryProvider: initialized");
    return true;
}

uint32_t WinApiMemoryProvider::GetDeviceCount() const
{
    return 2; // 0 = RAM, 1 = pagefile
}

void WinApiMemoryProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    m_status.dwLength = sizeof(MEMORYSTATUSEX);
    if (!GlobalMemoryStatusEx(&m_status))
        return;

    uint64_t total, free;

    if (deviceIndex == 0)
    {
        total = m_status.ullTotalPhys;
        free  = m_status.ullAvailPhys;
    }
    else if (deviceIndex == 1)
    {
        total = m_status.ullTotalPageFile;
        free  = m_status.ullAvailPageFile;
    }
    else return;

    double used    = static_cast<double>(total - free);
    double percent = total > 0 ? (used / total) * 100.0 : 0.0;

    snap.Set(static_cast<uint32_t>(MemoryMetric::Total),       static_cast<double>(total));
    snap.Set(static_cast<uint32_t>(MemoryMetric::Free),        static_cast<double>(free));
    snap.Set(static_cast<uint32_t>(MemoryMetric::Used),        used);
    snap.Set(static_cast<uint32_t>(MemoryMetric::UsedPercent), percent);
}

bool WinApiMemoryProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return false;
}
