#pragma once

#include <windows.h>
#include <pdh.h>

#include <vector>
#include <string>

#include "Categories/Storage/IStorageProvider.h"

#pragma comment(lib, "pdh.lib")

struct StorageDevice
{
    std::wstring instance;

    double readSpeed = 0;
    double writeSpeed = 0;

    uint64_t readBytes = 0;
    uint64_t writeBytes = 0;

    uint64_t freeSpace = 0;
    uint64_t totalSpace = 0;
};

class WinApiStorageProvider : public IStorageProvider
{
public:

    bool Initialize() override;

    void Update() override;

    uint32_t GetDeviceCount() const override;

    bool GetReadBytes(uint32_t device, double& value) override;
    bool GetWriteBytes(uint32_t device, double& value) override;

    bool GetReadSpeed(uint32_t device, double& value) override;
    bool GetWriteSpeed(uint32_t device, double& value) override;

    bool GetUsedSpace(uint32_t device, double& value) override;
    bool GetFreeSpaceBytes(uint32_t device, double& value) override;
    bool GetTotalSpace(uint32_t device, double& value) override;

private:

    PDH_HQUERY m_query = nullptr;

    PDH_HCOUNTER m_readCounter = nullptr;
    PDH_HCOUNTER m_writeCounter = nullptr;

    std::vector<StorageDevice> m_devices;

    void UpdateSpace(StorageDevice& device);
};