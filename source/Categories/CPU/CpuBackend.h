#pragma once

#include <windows.h>

#include "Core/BaseBackend.h"

class CpuBackend : public BaseBackend
{
public:
    double GetValue(uint32_t deviceIndex, uint32_t metricId) override;

protected:
    bool OnInitialize() override;
    void OnUpdate() override;

private:
    struct Snapshot
    {
        ULONGLONG idle = 0;
        ULONGLONG kernel = 0;
        ULONGLONG user = 0;
    };

    Snapshot m_prev{};
    Snapshot m_curr{};

    double m_usagePercent = 0.0;

    bool ReadSnapshot(Snapshot& snap);
};