#include "Providers/WinApi/WinApiStorageProvider.h"
#include "Types/StorageMetric.h"
#include "Utils/Debug.h"

#include <pdhmsg.h>
#include <algorithm>

#pragma comment(lib, "pdh.lib")

bool WinApiStorageProvider::Initialize()
{
    if (PdhOpenQuery(nullptr, 0, &m_query) != ERROR_SUCCESS)
    {
        LOG_STARTUP(L"WinApiStorageProvider: PdhOpenQuery failed");
        return false;
    }

    if (PdhAddEnglishCounter(m_query, L"\\PhysicalDisk(*)\\Disk Read Bytes/sec",  0, &m_readCounter)  != ERROR_SUCCESS ||
        PdhAddEnglishCounter(m_query, L"\\PhysicalDisk(*)\\Disk Write Bytes/sec", 0, &m_writeCounter) != ERROR_SUCCESS)
    {
        LOG_STARTUP(L"WinApiStorageProvider: failed to add PDH counters");
        return false;
    }

    // Two collections so PDH can compute an initial rate; names available after first
    PdhCollectQueryData(m_query);
    PdhCollectQueryData(m_query);

    // Discover devices now so GetDeviceCount() is correct before first GatherSnapshot
    DWORD bufSize = 0, count = 0;
    PdhGetFormattedCounterArray(m_readCounter, PDH_FMT_DOUBLE, &bufSize, &count, nullptr);
    if (bufSize > 0)
    {
        std::vector<BYTE> buf(bufSize);
        auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buf.data());
        PdhGetFormattedCounterArray(m_readCounter, PDH_FMT_DOUBLE, &bufSize, &count, items);
        EnsureDevicesDiscovered(count, items);
    }

    m_prevTime = GetTickCount64();

    LOG_STARTUP(L"WinApiStorageProvider: initialized (%u disk(s))", (uint32_t)m_devices.size());
    return true;
}

uint32_t WinApiStorageProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}

void WinApiStorageProvider::EnsureDevicesDiscovered(DWORD itemCount, PDH_FMT_COUNTERVALUE_ITEM* items)
{
    if (!m_devices.empty()) return;

    struct Entry { wchar_t drive; std::wstring name; };
    std::vector<Entry> entries;

    for (DWORD i = 0; i < itemCount; ++i)
    {
        std::wstring name = items[i].szName;
        if (name == L"_Total") continue;
        size_t pos = name.find(L':');
        if (pos == std::wstring::npos || pos == 0) continue;
        entries.push_back({ name[pos - 1], name });
    }

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.drive < b.drive; });

    for (auto& e : entries)
        m_devices.push_back({ e.name });
}

void WinApiStorageProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (!m_query) return;

    // Compute elapsed once per cycle (deviceIndex 0) so byte totals are time-correct
    if (deviceIndex == 0)
    {
        ULONGLONG now = GetTickCount64();
        double dt     = (now - m_prevTime) / 1000.0;
        m_elapsed     = dt > 0.0 ? dt : 1.0;
        m_prevTime    = now;
    }

    PdhCollectQueryData(m_query);

    DWORD bufSize = 0, count = 0;
    PdhGetFormattedCounterArray(m_readCounter, PDH_FMT_DOUBLE, &bufSize, &count, nullptr);
    if (bufSize == 0) return;

    std::vector<BYTE> buf(bufSize);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buf.data());

    if (PdhGetFormattedCounterArray(m_readCounter, PDH_FMT_DOUBLE, &bufSize, &count, items) != ERROR_SUCCESS) return;

    EnsureDevicesDiscovered(count, items);

    if (deviceIndex >= m_devices.size()) return;

    auto& dev = m_devices[deviceIndex];

    for (DWORD i = 0; i < count; ++i)
    {
        if (dev.instance == items[i].szName)
        {
            dev.readSpeed  = items[i].FmtValue.doubleValue;
            dev.readBytes += static_cast<uint64_t>(dev.readSpeed * m_elapsed);
            break;
        }
    }

    bufSize = 0; count = 0;
    PdhGetFormattedCounterArray(m_writeCounter, PDH_FMT_DOUBLE, &bufSize, &count, nullptr);
    if (bufSize == 0) return;

    buf.resize(bufSize);
    items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buf.data());

    if (PdhGetFormattedCounterArray(m_writeCounter, PDH_FMT_DOUBLE, &bufSize, &count, items) != ERROR_SUCCESS) return;

    for (DWORD i = 0; i < count; ++i)
    {
        if (dev.instance == items[i].szName)
        {
            dev.writeSpeed  = items[i].FmtValue.doubleValue;
            dev.writeBytes += static_cast<uint64_t>(dev.writeSpeed * m_elapsed);
            UpdateSpace(dev);
            break;
        }
    }

    snap.Set(static_cast<uint32_t>(StorageMetric::ReadSpeed),  dev.readSpeed);
    snap.Set(static_cast<uint32_t>(StorageMetric::WriteSpeed), dev.writeSpeed);
    snap.Set(static_cast<uint32_t>(StorageMetric::ReadBytes),  static_cast<double>(dev.readBytes));
    snap.Set(static_cast<uint32_t>(StorageMetric::WriteBytes), static_cast<double>(dev.writeBytes));
    snap.Set(static_cast<uint32_t>(StorageMetric::FreeSpace),  static_cast<double>(dev.freeSpace));
    snap.Set(static_cast<uint32_t>(StorageMetric::TotalSpace), static_cast<double>(dev.totalSpace));
    snap.Set(static_cast<uint32_t>(StorageMetric::UsedSpace),  static_cast<double>(dev.totalSpace - dev.freeSpace));
}

void WinApiStorageProvider::UpdateSpace(Device& dev)
{
    size_t pos = dev.instance.find(L':');
    if (pos == std::wstring::npos || pos == 0) return;

    std::wstring path = dev.instance.substr(pos - 1, 2) + L"\\";

    ULARGE_INTEGER freeBytes{}, totalBytes{};
    if (GetDiskFreeSpaceExW(path.c_str(), &freeBytes, &totalBytes, nullptr))
    {
        dev.freeSpace  = freeBytes.QuadPart;
        dev.totalSpace = totalBytes.QuadPart;
    }
}

bool WinApiStorageProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return false;
}
