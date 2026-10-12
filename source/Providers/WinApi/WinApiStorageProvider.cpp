#include "Providers/WinApi/WinApiStorageProvider.h"
#include "Types/StorageMetric.h"
#include "Utils/Debug.h"
#include "Utils/PdhUtil.h"

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

    // A PDH PhysicalDisk instance is "<disk number> <letter>:" - "1 C:" is volume C: on
    // PhysicalDrive1 - or just "<disk number>" for a disk carrying no mounted volume.
    struct Entry { wchar_t drive; int disk; std::wstring name; };
    std::vector<Entry> lettered, letterless;

    for (DWORD i = 0; i < itemCount; ++i)
    {
        std::wstring name = items[i].szName;
        if (name == L"_Total") continue;

        int disk = -1;
        if (!name.empty() && iswdigit(name[0]))
            disk = _wtoi(name.c_str());

        size_t pos = name.find(L':');
        if (pos == std::wstring::npos || pos == 0)
            letterless.push_back({ 0, disk, name });   // no volume: speeds and temperature only
        else
            lettered.push_back({ name[pos - 1], disk, name });
    }

    std::sort(lettered.begin(), lettered.end(),
              [](const Entry& a, const Entry& b) { return a.drive < b.drive; });
    std::sort(letterless.begin(), letterless.end(),
              [](const Entry& a, const Entry& b) { return a.disk < b.disk; });

    // Lettered volumes keep their existing order and therefore their existing Device=
    // indices; disks with no mounted volume are appended, so adding them breaks no skin.
    // They matter because a drive's temperature does not depend on it having a letter -
    // the only disk reporting one on the dev machine has none.
    for (auto& e : lettered)
    {
        Device d; d.instance = e.name; d.physicalDrive = e.disk;
        m_devices.push_back(d);
    }
    for (auto& e : letterless)
    {
        Device d; d.instance = e.name; d.physicalDrive = e.disk;
        m_devices.push_back(d);
    }
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

    if (!PdhReadCounterArray(m_readCounter, buf, items, count)) return;
    EnsureDevicesDiscovered(count, items);

    if (deviceIndex >= m_devices.size()) return;

    auto& dev = m_devices[deviceIndex];

    if (FindCounterValue(items, count, dev.instance, dev.readSpeed))
        dev.readBytes += static_cast<uint64_t>(dev.readSpeed * m_elapsed);

    if (PdhReadCounterArray(m_writeCounter, buf, items, count) &&
        FindCounterValue(items, count, dev.instance, dev.writeSpeed))
    {
        dev.writeBytes += static_cast<uint64_t>(dev.writeSpeed * m_elapsed);
        UpdateSpace(dev);
        UpdateIdentity(dev);
    }

    // Not inside the write-counter branch above: a disk carrying no mounted volume can
    // fail that lookup, and it is exactly the kind of disk whose temperature we want.
    UpdateTemperature(dev);

    if (PdhReadCounterArray(m_queueCounter, buf, items, count))
        FindCounterValue(items, count, dev.instance, dev.queueLength);

    double idlePercent = 0;
    if (PdhReadCounterArray(m_idleCounter, buf, items, count) &&
        FindCounterValue(items, count, dev.instance, idlePercent))
        dev.busyPercent = 100.0 - idlePercent;

    if (PdhReadCounterArray(m_readsPerSecCounter, buf, items, count))
        FindCounterValue(items, count, dev.instance, dev.readsPerSec);

    if (PdhReadCounterArray(m_writesPerSecCounter, buf, items, count))
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

    // Only published once a drive has actually answered - a drive whose firmware does
    // not implement it must read -1 (unsupported), not 0.
    if (dev.tempSupported > 0)
        snap.Set(static_cast<uint32_t>(StorageMetric::Temperature), dev.temperature);
}

// Drive temperature, the one temperature this plugin can read without a kernel driver.
// Declared here rather than taken from ntddstor.h so the build does not depend on the SDK
// being new enough; the layout is fixed and documented.
#ifndef StorageDeviceTemperatureProperty
#define StorageDeviceTemperatureProperty 52
#endif

struct NhmStorageTemperatureInfo
{
    USHORT Index;
    SHORT  Temperature;
    SHORT  OverThreshold;
    SHORT  UnderThreshold;
    UCHAR  OverThresholdChanged;
    UCHAR  UnderThresholdChanged;
};

struct NhmStorageTemperatureDescriptor
{
    ULONG  Version;
    ULONG  Size;
    SHORT  CriticalTemperature;
    SHORT  WarningTemperature;
    USHORT InfoCount;
    UCHAR  Reserved0[2];
    ULONG  Reserved1[2];
    NhmStorageTemperatureInfo TemperatureInfo[8];
};

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

void WinApiStorageProvider::UpdateTemperature(Device& dev)
{
    if (dev.tempSupported < 0 || dev.physicalDrive < 0)
        return;   // asked once and told no, or no disk number to ask about

    if (!dev.tempHandle)
    {
        wchar_t path[64];
        swprintf_s(path, L"\\\\.\\PhysicalDrive%d", dev.physicalDrive);

        // No access rights at all: enough for a property query, and it is what keeps this
        // working without administrator. Measured on an NVMe that answers.
        dev.tempHandle = CreateFileW(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     nullptr, OPEN_EXISTING, 0, nullptr);
        if (dev.tempHandle == INVALID_HANDLE_VALUE)
        {
            dev.tempHandle   = nullptr;
            dev.tempSupported = -1;
            return;
        }
    }

    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = static_cast<STORAGE_PROPERTY_ID>(StorageDeviceTemperatureProperty);
    query.QueryType  = PropertyStandardQuery;

    NhmStorageTemperatureDescriptor desc{};
    DWORD returned = 0;

    if (!DeviceIoControl(dev.tempHandle, IOCTL_STORAGE_QUERY_PROPERTY,
                         &query, sizeof(query), &desc, sizeof(desc), &returned, nullptr))
    {
        // Firmware that does not implement it answers ERROR_IO_DEVICE. Support is per
        // drive, not per bus - of two NVMe drives in the dev machine, one answers.
        dev.tempSupported = -1;
        CloseHandle(dev.tempHandle);
        dev.tempHandle = nullptr;
        return;
    }

    // InfoCount is not to be trusted: a drive reporting 3 gave one real sensor plus
    // 0 C with no thresholds and SHRT_MIN under an index of 65262. Take the lowest
    // index that carries a plausible reading.
    const SHORT NoReading = -32768;
    int  best     = -1;
    bool haveBest = false;

    for (USHORT i = 0; i < desc.InfoCount && i < 8; ++i)
    {
        const NhmStorageTemperatureInfo& info = desc.TemperatureInfo[i];
        if (info.Temperature == NoReading || info.Temperature <= 0 || info.Temperature > 150)
            continue;
        if (!haveBest)
        {
            best     = info.Temperature;
            haveBest = true;
        }
    }

    if (!haveBest)
    {
        dev.tempSupported = -1;
        return;
    }

    dev.tempSupported = 1;
    dev.temperature   = static_cast<double>(best);
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
        UINT type       = GetDriveTypeW(path.c_str());
        dev.volumeLabel = label;
        dev.fileSystem  = fileSystem;
        dev.driveType   = DriveTypeToString(type);
        dev.identityRead = true;

        // Unlabeled drive: match Explorer's default display name instead of "".
        if (dev.volumeLabel.empty())
            dev.volumeLabel = (type == DRIVE_REMOVABLE) ? L"Removable Disk" : L"Local Disk";
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
