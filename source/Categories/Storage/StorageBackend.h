#pragma once

#include <memory>
#include <vector>

#include "Core/ICategoryBackend.h"
#include "Categories/Storage/IStorageProvider.h"
#include "Types/StorageMetric.h"

class StorageBackend : public ICategoryBackend
{
public:

    bool Initialize() override;

    void Update() override;

    double GetValue(uint32_t metricId, uint32_t deviceIndex) override;

    uint32_t GetDeviceCount() const override;

private:

    std::vector<std::unique_ptr<IStorageProvider>> m_providers;

    bool m_initialized = false;
    bool m_initAttempted = false;
    bool m_initFailed = false;
};