#pragma once

#include "Core/BaseBackend.h"
#include "Types/CpuMetric.h"
#include "Categories/CPU/ICpuProvider.h"

#include <vector>
#include <memory>

class CpuBackend : public BaseBackend
{
public:
    double GetValue(uint32_t metricId, uint32_t deviceIndex) override;

    uint32_t GetDeviceCount() const override
    {
        return m_deviceCount;
    }


protected:
    bool OnInitialize() override;
    void OnUpdate() override;

private:
    std::vector<std::unique_ptr<ICpuProvider>> m_providers;

    ICpuProvider* m_provider = nullptr;

    uint32_t m_deviceCount = 0;

    bool m_loggedUnsupportedTemp = false;
    bool m_loggedUnknownMetric = false;
};