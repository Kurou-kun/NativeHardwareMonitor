#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <winsock2.h>
#include <windows.h>
#include <netioapi.h>
#include <vector>

class WinApiNetworkProvider : public IProvider
{
public:
    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    struct Device
    {
        NET_LUID luid{};
        uint64_t prevRx = 0, prevTx = 0;
        uint64_t rxTotal = 0, txTotal = 0;
        double   rxSpeed = 0, txSpeed = 0;
        uint64_t linkSpeed = 0;
    };

    std::vector<Device> m_devices;
    ULONGLONG           m_prevTime  = 0;
    double              m_deltaTime = 1.0;
};
