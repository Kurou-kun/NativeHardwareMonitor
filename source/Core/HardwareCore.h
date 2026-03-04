#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>

#include "Types/Category.h"
#include "Core/ICategoryBackend.h"

class HardwareCore
{
public:
    static HardwareCore& Instance();

    uint32_t RegisterMeasure(
        Category category,
        uint32_t metricId,
        uint32_t deviceIndex
    );

    void UnregisterMeasure(uint32_t handle);

    double GetValue(uint32_t handle);

private:
    HardwareCore();
    ~HardwareCore() = default;

    HardwareCore(const HardwareCore&) = delete;
    HardwareCore& operator=(const HardwareCore&) = delete;

    struct MeasureEntry
    {
        Category category;
        uint32_t metricId;
        uint32_t deviceIndex;
    };

    struct BackendState
    {
        std::unique_ptr<ICategoryBackend> backend;

        uint64_t lastUpdateTime = 0;
        uint32_t minIntervalMs = 100; // twarde minimum
    };

    std::unordered_map<uint32_t, MeasureEntry> m_measures;
    std::unordered_map<Category, BackendState> m_backends;

    uint32_t m_nextHandle = 1;

    BackendState* GetBackendState(Category category);

    uint64_t GetTimeMs() const;
};