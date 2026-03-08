#pragma once

#include "Core/ICategoryBackend.h"
#include "Categories/Memory/IMemoryProvider.h"

#include <memory>
#include <vector>

class MemoryBackend : public ICategoryBackend
{
public:

    bool Initialize() override;

    void Update() override;

    double GetValue(uint32_t metricId, uint32_t deviceIndex) override;

    uint32_t GetDeviceCount() const override;

private:

    std::vector<std::unique_ptr<IMemoryProvider>> m_providers;

    bool m_initAttempted = false;
    bool m_initFailed = false;
};