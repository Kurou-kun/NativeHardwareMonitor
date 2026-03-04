#include "Categories/Storage/StorageBackend.h"
#include "Types/StorageMetric.h"

bool StorageBackend::OnInitialize()
{
    m_prevTime = GetTickCount64();
    return ReadSnapshot(m_prev);
}

void StorageBackend::OnUpdate()
{
    uint64_t now = GetTickCount64();
    uint64_t deltaMs = now - m_prevTime;

    if (deltaMs == 0)
        return;

    if (!ReadSnapshot(m_curr))
        return;

    uint64_t readDiff = m_curr.readBytes - m_prev.readBytes;
    uint64_t writeDiff = m_curr.writeBytes - m_prev.writeBytes;

    m_readPerSec = (double)readDiff * 1000.0 / deltaMs;
    m_writePerSec = (double)writeDiff * 1000.0 / deltaMs;

    m_prev = m_curr;
    m_prevTime = now;
}

double StorageBackend::GetValue(uint32_t, uint32_t metricId)
{
    if (metricId == static_cast<uint32_t>(StorageMetric::ReadBytesPerSec))
        return m_readPerSec;

    if (metricId == static_cast<uint32_t>(StorageMetric::WriteBytesPerSec))
        return m_writePerSec;

    return 0.0;
}

bool StorageBackend::ReadSnapshot(Snapshot& snap)
{
    DWORD drives = GetLogicalDrives();

    uint64_t totalRead = 0;
    uint64_t totalWrite = 0;

    for (int i = 0; i < 26; ++i)
    {
        if (!(drives & (1 << i)))
            continue;

        wchar_t path[] = L"\\\\.\\A:";
        path[4] = L'A' + i;

        HANDLE hDisk = CreateFile(
            path,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hDisk == INVALID_HANDLE_VALUE)
            continue;

        DISK_PERFORMANCE perf = {};
        DWORD bytesReturned = 0;

        if (DeviceIoControl(
            hDisk,
            IOCTL_DISK_PERFORMANCE,
            nullptr,
            0,
            &perf,
            sizeof(perf),
            &bytesReturned,
            nullptr))
        {
            totalRead += perf.BytesRead.QuadPart;
            totalWrite += perf.BytesWritten.QuadPart;
        }

        CloseHandle(hDisk);
    }

    snap.readBytes = totalRead;
    snap.writeBytes = totalWrite;

    return true;
}