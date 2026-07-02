#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>

class WinApiMemoryProvider : public IProvider
{
public:
    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    MEMORYSTATUSEX m_status{};
};
