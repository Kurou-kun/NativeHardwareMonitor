#pragma once

#include <vector>

#include "Categories/GPU/IGpuProvider.h"
#include "Categories/GPU/NvidiaLoader.h"

class NvidiaProvider : public IGpuProvider
{
public:
    bool Initialize() override;
    uint32_t GetDeviceCount() const override;

    bool GetUtilization(uint32_t index, double& value) override;
    bool GetMemoryUsed(uint32_t index, uint64_t& value) override;

private:
    NvidiaLoader m_loader;
    uint32_t m_deviceCount = 0;
};