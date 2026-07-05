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
        PdhAddEnglishCounter(m_query, L"\\PhysicalDisk(*)\\Disk Write Bytes/sec", 0, &m_writeCounter) != ERROR_SUCCESS ||
        PdhAddEnglishCounter(m_query, L"\\PhysicalDisk(*)\\Avg. Disk Queue Length", 0, &m_queueCounter) != ERROR_SUCCESS ||
        PdhAddEnglishCounter(m_query, L"\\PhysicalDisk(*)\\% Idle Time",         0, &m_idleCounter)  != ERROR_SUCCESS ||
        PdhAddEnglishCounter(m_query, L"\\PhysicalDisk(*)\\Disk Reads/sec",      0, &m_readsPerSecCounter)  != ERROR_SUCCESS ||
        PdhAddEnglishCounter(m_query, L"\\PhysicalDisk(*)\\Disk Writes/sec",     0, &m_writesPerSecCounter) != ERROR_SUCCESS)
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

bool WinApiStorageProvider::FindCounterValue(PDH_FMT_COUNTERVALUE_ITEM* items, DWORD count,
                                              const std::wstring& instance, double& out)
{
    for (DWORD i = 0; i < count; ++i)
    {
        if (instance == items[i].szName)
        {
            out = items[i].FmtValue.doubleValue;
            return true;
        }
    }
    return false;
}

static bool ReadCounterArray(PDH_HCOUNTER counter, std::vector<BYTE>& buf,
                              PDH_FMT_COUNTERVALUE_ITEM*& items, DWORD& count)
{
    DWORD bufSize = 0;
    PdhGetFormattedCounterArray(counter, PDH_FMT_DOUBLE, &bufSize, &count, nullptr);
    if (bufSize == 0) return false;

    buf.resize(bufSize);
    items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buf.data());

    return PdhGetFormattedCounterArray(counter, PDH_FMT_DOUBLE, &bufSize, &count, items) == ERROR_SUCCESS;
}

void WinApiStorageProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (!m_query) return;

    // Once per cycle (deviceIndex 0): elapsed for byte totals, and the PDH collect.
    // PDH rates are computed between the last two collects — collecting per device
    // would give every disk after the first a near-zero interval and garbage rates.
    if (deviceIndex == 0)
    {
        ULONGLONG now = GetTickCount64();
        double dt     = (now - m_prevTime) / 1000.0;
        m_elapsed     = dt > 0.0 ? dt : 1.0;
        m_prevTime    = now;

        PdhCollectQueryData(m_query);
    }

    std::vector<BYTE> buf;
    PDH_FMT_COUNTERVALUE_ITEM* items = nullptr;
    DWORD count = 0;

    if (!ReadCounterArray(m_readCounter, buf, items, count)) return;
    EnsureDevicesDiscovered(count, items);

    if (deviceIndex >= m_devices.size()) return;

    auto& dev = m_devices[deviceIndex];

    if (FindCounterValue(items, count, dev.instance, dev.readSpeed))
        dev.readBytes += static_cast<uint64_t>(dev.readSpeed * m_elapsed);

    if (ReadCounterArray(m_writeCounter, buf, items, count) &&
        FindCounterValue(items, count, dev.instance, dev.writeSpeed))
    {
        dev.writeBytes += static_cast<uint64_t>(dev.writeSpeed * m_elapsed);
        UpdateSpace(dev);
        UpdateIdentity(dev);
    }

    if (ReadCounterArray(m_queueCounter, buf, items, count))
        FindCounterValue(items, count, dev.instance, dev.queueLength);

    double idlePercent = 0;
    if (ReadCounterArray(m_idleCounter, buf, items, count) &&
        FindCounterValue(items, count, dev.instance, idlePercent))
        dev.busyPercent = 100.0 - idlePercent;

    if (ReadCounterArray(m_readsPerSecCounter, buf, items, count))
        FindCounterValue(items, count, dev.instance, dev.readsPerSec);

    if (ReadCounterArray(m_writesPerSecCounter, buf, items, count))
        FindCounterValue(items, count, dev.instance, dev.writesPerSec);

    snap.Set(static_cast<uint32_t>(StorageMetric::ReadSpeed),    dev.readSpeed);
    snap.Set(static_cast<uint32_t>(StorageMetric::WriteSpeed),   dev.writeSpeed);
    snap.Set(static_cast<uint32_t>(StorageMetric::ReadBytes),    static_cast<double>(dev.readBytes));
    snap.Set(static_cast<uint32_t>(StorageMetric::WriteBytes),   static_cast<double>(dev.writeBytes));
    snap.Set(static_cast<uint32_t>(StorageMetric::FreeSpace),    static_cast<double>(dev.freeSpace));
    snap.Set(static_cast<uint32_t>(StorageMetric::TotalSpace),   static_cast<double>(dev.totalSpace));
    snap.Set(static_cast<uint32_t>(StorageMetric::UsedSpace),    static_cast<double>(dev.totalSpace - dev.freeSpace));
    snap.Set(static_cast<uint32_t>(StorageMetric::QueueLength),  dev.queueLength);
    snap.Set(static_cast<uint32_t>(StorageMetric::BusyPercent),  dev.busyPercent);
    snap.Set(static_cast<uint32_t>(StorageMetric::ReadsPerSec),  dev.readsPerSec);
    snap.Set(static_cast<uint32_t>(StorageMetric::WritesPerSec), dev.writesPerSec);
}

static std::wstring DrivePath(const std::wstring& instance)
{
    size_t pos = instance.find(L':');
    if (pos == std::wstring::npos || pos == 0) return L"";

    return instance.substr(pos - 1, 2) + L"\\";
}

void WinApiStorageProvider::UpdateSpace(Device& dev)
{
    std::wstring path = DrivePath(dev.instance);
    if (path.empty()) return;

    ULARGE_INTEGER freeBytes{}, totalBytes{};
    if (GetDiskFreeSpaceExW(path.c_str(), &freeBytes, &totalBytes, nullptr))
    {
        dev.freeSpace  = freeBytes.QuadPart;
        dev.totalSpace = totalBytes.QuadPart;
    }
}

static const wchar_t* DriveTypeToString(UINT type)
{
    switch (type)
    {
    case DRIVE_REMOVABLE: return L"Removable";
    case DRIVE_FIXED:     return L"Fixed";
    case DRIVE_REMOTE:    return L"Network";
    case DRIVE_CDROM:     return L"CD-ROM";
    case DRIVE_RAMDISK:   return L"RAM Disk";
    default:              return L"Unknown";
    }
}

void WinApiStorageProvider::UpdateIdentity(Device& dev)
{
    if (dev.identityRead) return;

    std::wstring path = DrivePath(dev.instance);
    if (path.empty()) return;

    wchar_t label[MAX_PATH + 1]      = {};
    wchar_t fileSystem[MAX_PATH + 1] = {};

    if (GetVolumeInformationW(path.c_str(), label, MAX_PATH, nullptr, nullptr, nullptr, fileSystem, MAX_PATH))
    {
        dev.volumeLabel = label;
        dev.fileSystem  = fileSystem;
        dev.driveType   = DriveTypeToString(GetDriveTypeW(path.c_str()));
        dev.identityRead = true;
    }
}

bool WinApiStorageProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_devices.size())
        return false;

    auto& dev = m_devices[deviceIndex];

    switch (static_cast<StorageMetric>(metricId))
    {
    case StorageMetric::VolumeLabel: if (!dev.identityRead) return false; out = dev.volumeLabel; return true;
    case StorageMetric::FileSystem:  if (!dev.identityRead) return false; out = dev.fileSystem;  return true;
    case StorageMetric::DriveType:   if (!dev.identityRead) return false; out = dev.driveType;   return true;
    default:                         return false;
    }
}
