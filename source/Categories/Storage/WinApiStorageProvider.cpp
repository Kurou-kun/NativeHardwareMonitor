#include "Categories/Storage/WinApiStorageProvider.h"
#include "Utils/Debug.h"

#include <pdhmsg.h>
#include <algorithm>
#include <sstream>

bool WinApiStorageProvider::Initialize()
{
    LOG_INFO(L"[StorageProvider] Initialize");

    if (PdhOpenQuery(nullptr, 0, &m_query) != ERROR_SUCCESS)
    {
        LOG_ERROR(L"[StorageProvider] PdhOpenQuery failed");
        return false;
    }

    if (PdhAddEnglishCounter(
        m_query,
        L"\\PhysicalDisk(*)\\Disk Read Bytes/sec",
        0,
        &m_readCounter) != ERROR_SUCCESS)
    {
        LOG_ERROR(L"[StorageProvider] Failed to add read counter");
        return false;
    }

    if (PdhAddEnglishCounter(
        m_query,
        L"\\PhysicalDisk(*)\\Disk Write Bytes/sec",
        0,
        &m_writeCounter) != ERROR_SUCCESS)
    {
        LOG_ERROR(L"[StorageProvider] Failed to add write counter");
        return false;
    }

    // Baseline sample
    PdhCollectQueryData(m_query);

    LOG_INFO(L"[StorageProvider] PDH counters initialized");

    return true;
}

void WinApiStorageProvider::Update()
{
    if (!m_query)
        return;

    PdhCollectQueryData(m_query);

    DWORD bufferSize = 0;
    DWORD itemCount = 0;

    PdhGetFormattedCounterArray(
        m_readCounter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        nullptr);

    if (bufferSize == 0)
        return;

    std::vector<BYTE> buffer(bufferSize);

    auto* items =
        reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buffer.data());

    if (PdhGetFormattedCounterArray(
        m_readCounter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        items) != ERROR_SUCCESS)
        return;

    if (m_devices.empty())
    {
        struct DiskInstance
        {
            wchar_t drive;
            std::wstring name;
        };

        std::vector<DiskInstance> instances;

        for (DWORD i = 0; i < itemCount; i++)
        {
            std::wstring name = items[i].szName;

            if (name == L"_Total")
                continue;

            size_t pos = name.find(L':');
            if (pos == std::wstring::npos || pos == 0)
                continue;

            wchar_t drive = name[pos - 1];

            instances.push_back({ drive, name });
        }

        std::sort(instances.begin(), instances.end(),
            [](const DiskInstance& a, const DiskInstance& b)
            {
                return a.drive < b.drive;
            });

        for (const auto& inst : instances)
        {
            StorageDevice device;
            device.instance = inst.name;

            m_devices.push_back(device);

            LOG_INFO(
                L"[StorageProvider] Detected disk instance: %ls (Device=%zu)",
                inst.name.c_str(),
                m_devices.size() - 1
            );
        }
    }

    for (DWORD i = 0; i < itemCount; i++)
    {
        std::wstring name = items[i].szName;

        if (name == L"_Total")
            continue;

        for (auto& device : m_devices)
        {
            if (device.instance == name)
            {
                device.readSpeed = items[i].FmtValue.doubleValue;
                device.readBytes += static_cast<uint64_t>(device.readSpeed);
                break;
            }
        }
    }

    bufferSize = 0;
    itemCount = 0;

    PdhGetFormattedCounterArray(
        m_writeCounter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        nullptr);

    if (bufferSize == 0)
        return;

    buffer.resize(bufferSize);

    items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buffer.data());

    if (PdhGetFormattedCounterArray(
        m_writeCounter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        items) != ERROR_SUCCESS)
        return;

    for (DWORD i = 0; i < itemCount; i++)
    {
        std::wstring name = items[i].szName;

        if (name == L"_Total")
            continue;

        for (auto& device : m_devices)
        {
            if (device.instance == name)
            {
                device.writeSpeed = items[i].FmtValue.doubleValue;
                device.writeBytes += static_cast<uint64_t>(device.writeSpeed);

                UpdateSpace(device);

                break;
            }
        }
    }
}

void WinApiStorageProvider::UpdateSpace(StorageDevice& device)
{
    const std::wstring& instance = device.instance;

    size_t colonPos = instance.find(L':');

    if (colonPos == std::wstring::npos || colonPos == 0)
        return;

    std::wstring drive = instance.substr(colonPos - 1, 2);

    std::wstring path = drive + L"\\";

    ULARGE_INTEGER freeBytes;
    ULARGE_INTEGER totalBytes;

    if (GetDiskFreeSpaceExW(
        path.c_str(),
        &freeBytes,
        &totalBytes,
        nullptr))
    {
        device.freeSpace = freeBytes.QuadPart;
        device.totalSpace = totalBytes.QuadPart;
    }
}

bool WinApiStorageProvider::GetReadBytes(uint32_t device, double& value)
{
    if (device >= m_devices.size())
        return false;

    value = static_cast<double>(m_devices[device].readBytes);
    return true;
}

bool WinApiStorageProvider::GetWriteBytes(uint32_t device, double& value)
{
    if (device >= m_devices.size())
        return false;

    value = static_cast<double>(m_devices[device].writeBytes);
    return true;
}

bool WinApiStorageProvider::GetReadSpeed(uint32_t device, double& value)
{
    if (device >= m_devices.size())
        return false;

    value = m_devices[device].readSpeed;
    return true;
}

bool WinApiStorageProvider::GetWriteSpeed(uint32_t device, double& value)
{
    if (device >= m_devices.size())
        return false;

    value = m_devices[device].writeSpeed;
    return true;
}

bool WinApiStorageProvider::GetUsedSpace(uint32_t device, double& value)
{
    if (device >= m_devices.size())
        return false;

    const auto& d = m_devices[device];

    value = static_cast<double>(d.totalSpace - d.freeSpace);
    return true;
}

bool WinApiStorageProvider::GetFreeSpaceBytes(uint32_t device, double& value)
{
    if (device >= m_devices.size())
        return false;

    value = static_cast<double>(m_devices[device].freeSpace);
    return true;
}

bool WinApiStorageProvider::GetTotalSpace(uint32_t device, double& value)
{
    if (device >= m_devices.size())
        return false;

    value = static_cast<double>(m_devices[device].totalSpace);
    return true;
}

uint32_t WinApiStorageProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}