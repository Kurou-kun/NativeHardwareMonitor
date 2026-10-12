#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Types/Category.h"
#include "Core/IModule.h"
#include "Core/PollerThread.h"

class HardwareCore
{
public:
    static HardwareCore& Instance();

    uint32_t RegisterMeasure(
        Category             category,
        uint32_t              metricId,
        uint32_t              deviceIndex,
        uint32_t              updateOverrideMs,
        const std::wstring&  targetHost     = L"",
        uint32_t              pingIntervalMs = 0);
    void     UnregisterMeasure(uint32_t handle);

    double GetValue(uint32_t handle);
    bool   GetString(uint32_t handle, std::wstring& out);

    // Debug=2 diagnostic: dump every metric for one category+device to a UTF-8 file.
    void   DumpDevice(Category category, uint32_t deviceIndex, const std::wstring& path);

    // Device count for a category, so a dump can cover devices no measure reads.
    uint32_t DeviceCount(Category category);

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

        // Last value handed to Rainmeter. Returned again when a gather holds the
        // lock, so a reader never waits on hardware (see GetValue).
        double   lastValue = -1.0;
    };

    struct ModuleState
    {
        std::unique_ptr<IModule>      module;
        std::timed_mutex              mutex;   // must outlive poller (destroyed after it)
        std::unique_ptr<PollerThread> poller;  // reset signals the thread; it is never joined

        // Device count, published by the poller once the module is up. Read without
        // the mutex and never negative: skins clamp a device index against it
        // (Clamp(x, 0, count-1)), so a transient -1 here makes the clamp yield -2 and
        // the skin then reads a device that cannot exist. Reported by Yincognito
        // scrolling sectionvariables.ini quickly.
        std::atomic<uint32_t> deviceCount{ 0 };

        // Provider liveness, published by the poller and read without the mutex for the
        // same reason the device count is: it answers what -1 on a metric cannot, so it
        // must not itself be able to come back as a stale -1 from the try_lock path.
        // 0 until the poller has run, then 1 (gathering) or -1 (no provider here).
        std::atomic<int32_t> status{ 0 };

        uint32_t currentOverrideMs = 0;
        bool     initialized       = false;

        std::unordered_map<uint32_t, uint32_t> overrides; // handle → ms

        // Device indices some measure actually reads. The poller gathers only these;
        // empty means everything, which is also what a Debug=2 dump needs - so a dump
        // sets dumpAll and the category goes back to gathering the lot from then on.
        std::vector<uint32_t> wantedDevices;
        bool                  dumpAll = false;
    };

    std::unordered_map<uint32_t, MeasureEntry>                 m_measures;
    std::unordered_map<Category, std::unique_ptr<ModuleState>> m_modules;

    uint32_t m_nextHandle = 1;

    ModuleState* GetModuleState(Category category);
    void         UpdatePoller(ModuleState& state);
    void         RebuildWantedDevices(Category category);
    void         EnsurePoller(ModuleState& state);
    void         StartPoller(ModuleState& state, uint32_t intervalMs);
};
