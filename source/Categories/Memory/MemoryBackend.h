#pragma once

#include <windows.h>

#include "Core/BaseBackend.h"

class MemoryBackend : public BaseBackend
{
public:
    double GetValue(uint32_t deviceIndex, uint32_t metricId) override;

protected:
    bool OnInitialize() override;
    void OnUpdate() override;

private:
    struct Snapshot
    {
        uint64_t totalBytes = 0;
        uint64_t availBytes = 0;
    };

    Snapshot m_snapshot;
};