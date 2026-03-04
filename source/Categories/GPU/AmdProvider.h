#pragma once

#include "IGpuProvider.h"
#include "ADLXLoader.h"

#include "ADLXHelper.h"
#include "IGPU.h"
#include "IPerformanceMonitoring.h"

class AmdProvider : public IGpuProvider
{
public:
    AmdProvider();
    ~AmdProvider() override;

    bool Initialize() override;
    uint32_t GetDeviceCount() const override;

    bool GetUtilization(uint32_t index, double& value) override;
    bool GetMemoryUsed(uint32_t index, uint64_t& value) override;

private:
    ADLXLoader m_loader;
    bool m_initialized = false;
};