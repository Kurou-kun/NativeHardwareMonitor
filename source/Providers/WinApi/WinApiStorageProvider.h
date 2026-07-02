#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <pdh.h>
#include <vector>
#include <string>

class WinApiStorageProvider : public IProvider
{
public:
    ~WinApiStorageProvider() { if (m_query) PdhCloseQuery(m_query); }

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    struct Device
    {
        std::wstring instance;
        double   readSpeed  = 0, writeSpeed = 0;
        uint64_t readBytes  = 0, writeBytes = 0;
        uint64_t freeSpace  = 0, totalSpace = 0;
    };

    PDH_HQUERY   m_query        = nullptr;
    PDH_HCOUNTER m_readCounter  = nullptr;
    PDH_HCOUNTER m_writeCounter = nullptr;

    std::vector<Device> m_devices;
    ULONGLONG           m_prevTime = 0;
    double              m_elapsed  = 1.0;

    void UpdateSpace(Device& dev);
    void EnsureDevicesDiscovered(DWORD itemCount, PDH_FMT_COUNTERVALUE_ITEM* items);
};
