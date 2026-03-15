#pragma once

#include "Categories/Network/INetworkProvider.h"

#include <winsock2.h>
#include <windows.h>
#include <netioapi.h>

#include <vector>

#pragma comment(lib, "iphlpapi.lib")

class WinApiNetworkProvider : public INetworkProvider
{
public:

    bool Initialize() override;

    void Update() override;

    bool GetDownload(uint32_t deviceIndex, double& value) override;

    bool GetUpload(uint32_t deviceIndex, double& value) override;

    bool GetDownloadTotal(uint32_t deviceIndex, double& value) override;

    bool GetUploadTotal(uint32_t deviceIndex, double& value) override;

    bool GetSpeed(uint32_t deviceIndex, double& value) override;

    uint32_t GetDeviceCount() const override;

private:

    struct NetworkDevice
    {
        NET_LUID luid{};

        uint64_t prevRx = 0;
        uint64_t prevTx = 0;

        uint64_t rxTotal = 0;
        uint64_t txTotal = 0;

        double rxSpeed = 0;
        double txSpeed = 0;

        uint64_t linkSpeed = 0;
    };

    std::vector<NetworkDevice> m_devices;

    ULONGLONG m_prevTime = 0;
};