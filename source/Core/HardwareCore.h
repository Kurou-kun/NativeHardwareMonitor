#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Types/Category.h"
#include "Core/IModule.h"
#include "Core/PollerThread.h"

class HardwareCore
{
public:
    static HardwareCore& Instance();

    uint32_t RegisterMeasure(Category category, uint32_t metricId, uint32_t deviceIndex, uint32_t updateOverrideMs);
    void     UnregisterMeasure(uint32_t handle);

    double GetValue(uint32_t handle);
    bool   GetString(uint32_t handle, std::wstring& out);

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
        uint32_t updateOverrideMs;
    };

    struct ModuleState
    {
        std::unique_ptr<IModule>      module;
        std::mutex                    mutex;   // must outlive poller (destroyed after it)
        std::unique_ptr<PollerThread> poller;  // destroyed first: Stop() joins the thread

        uint64_t lastGatherTime    = 0;
        uint32_t minIntervalMs     = 100;
        uint32_t currentOverrideMs = 0;
        bool     initialized       = false;

        std::unordered_map<uint32_t, uint32_t> overrides; // handle → ms
    };

    std::unordered_map<uint32_t, MeasureEntry>                 m_measures;
    std::unordered_map<Category, std::unique_ptr<ModuleState>> m_modules;

    uint32_t m_nextHandle = 1;

    ModuleState* GetModuleState(Category category);
    void         UpdatePoller(ModuleState& state);
    uint64_t     GetTimeMs() const;
};
