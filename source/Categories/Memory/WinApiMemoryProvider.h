#pragma once

#include "Categories/Memory/IMemoryProvider.h"

#include <windows.h>

class WinApiMemoryProvider : public IMemoryProvider
{
public:

    bool Initialize() override;

    void Update() override;

    bool GetUsed(uint32_t deviceIndex, double& value) override;

    bool GetFree(uint32_t deviceIndex, double& value) override;

    bool GetTotal(uint32_t deviceIndex, double& value) override;

    bool GetUsedPercent(uint32_t deviceIndex, double& value) override;

    uint32_t GetDeviceCount() const override;

private:

    MEMORYSTATUSEX m_memory{};

    uint64_t m_ramTotal = 0;
    uint64_t m_ramFree = 0;

    uint64_t m_swapTotal = 0;
    uint64_t m_swapFree = 0;
};