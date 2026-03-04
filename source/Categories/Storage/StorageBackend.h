#pragma once

#include <windows.h>
#include <winioctl.h>

#include "Core/BaseBackend.h"

class StorageBackend : public BaseBackend
{
public:
    double GetValue(uint32_t deviceIndex, uint32_t metricId) override;

protected:
    bool OnInitialize() override;
    void OnUpdate() override;

private:
    struct Snapshot
    {
        uint64_t readBytes = 0;
        uint64_t writeBytes = 0;
    };

    Snapshot m_prev{};
    Snapshot m_curr{};

    double m_readPerSec = 0.0;
    double m_writePerSec = 0.0;

    uint64_t m_prevTime = 0;

    bool ReadSnapshot(Snapshot& snap);
};