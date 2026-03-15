#pragma once

#include "Core/ICategoryBackend.h"
#include "Categories/Network/INetworkProvider.h"

#include <memory>
#include <vector>

class NetworkBackend : public ICategoryBackend
{
public:

    bool Initialize() override;

    void Update() override;

    double GetValue(uint32_t metricId, uint32_t deviceIndex) override;

    uint32_t GetDeviceCount() const override;

private:

    std::vector<std::unique_ptr<INetworkProvider>> m_providers;

    bool m_initAttempted = false;
    bool m_initFailed = false;
};