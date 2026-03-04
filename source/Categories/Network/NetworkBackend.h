#pragma once

#include <winsock2.h> 
#include <windows.h>
#include <netioapi.h>
#include <iphlpapi.h>

#include "Core/BaseBackend.h"

#pragma comment(lib, "iphlpapi.lib")

class NetworkBackend : public BaseBackend
{
public:
    double GetValue(uint32_t deviceIndex, uint32_t metricId) override;

protected:
    bool OnInitialize() override;
    void OnUpdate() override;

private:
    struct Snapshot
    {
        uint64_t rxBytes = 0;
        uint64_t txBytes = 0;
    };

    Snapshot m_prev{};
    Snapshot m_curr{};

    double m_rxPerSec = 0.0;
    double m_txPerSec = 0.0;

    uint64_t m_prevTime = 0;

    bool ReadSnapshot(Snapshot& snap);
};