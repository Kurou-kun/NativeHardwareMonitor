#include "Core/HardwareCore.h"

#include "Modules/GPU/GpuModule.h"
#include "Modules/CPU/CpuModule.h"
#include "Modules/Memory/MemoryModule.h"
#include "Modules/Network/NetworkModule.h"
#include "Modules/Storage/StorageModule.h"
#include "Modules/Ping/PingModule.h"

#include <algorithm>
#include <windows.h>

HardwareCore& HardwareCore::Instance()
{
    static HardwareCore instance;
    return instance;
}

HardwareCore::HardwareCore()
{
    auto make = [](auto mod) {
        auto s = std::make_unique<ModuleState>();
        s->module = std::move(mod);
        return s;
    };

    m_modules[Category::GPU]     = make(std::make_unique<GpuModule>());
    m_modules[Category::CPU]     = make(std::make_unique<CpuModule>());
    m_modules[Category::Memory]  = make(std::make_unique<MemoryModule>());
    m_modules[Category::Network] = make(std::make_unique<NetworkModule>());
    m_modules[Category::Storage] = make(std::make_unique<StorageModule>());
    m_modules[Category::Ping]    = make(std::make_unique<PingModule>());
}

uint32_t HardwareCore::RegisterMeasure(
    Category            category,
    uint32_t             metricId,
    uint32_t             deviceIndex,
    uint32_t             updateOverrideMs,
    const std::wstring& targetHost,
    uint32_t             pingIntervalMs)
{
    uint32_t handle = m_nextHandle++;

    if (category == Category::Ping)
    {
        // Ping devices are resolved from a host string, not a skin-supplied
        // Device= index — and unlike other categories, it must be initialized
        // here (at registration time) rather than lazily on first read, since
        // the background ping polling has to start the moment the skin loads.
        ModuleState* state = GetModuleState(category);
        if (state && state->module)
        {
            // ResolveTarget grows the module's snapshot vector — must not race
            // a PollerThread that's mid-GatherAll on this module.
            std::lock_guard<std::mutex> lock(state->mutex);

            if (!state->initialized)
                state->initialized = state->module->Initialize();

            if (state->initialized)
                deviceIndex = state->module->ResolveTarget(targetHost, pingIntervalMs);
        }
    }

    m_measures.emplace(handle, MeasureEntry{ category, metricId, deviceIndex, updateOverrideMs });

    if (updateOverrideMs > 0)
    {
        ModuleState* state = GetModuleState(category);
        if (state)
        {
            state->overrides[handle] = updateOverrideMs;
            UpdatePoller(*state);
        }
    }

    return handle;
}

void HardwareCore::UnregisterMeasure(uint32_t handle)
{
    auto it = m_measures.find(handle);
    if (it == m_measures.end())
        return;

    const MeasureEntry& entry = it->second;

    if (entry.updateOverrideMs > 0)
    {
        ModuleState* state = GetModuleState(entry.category);
        if (state)
        {
            state->overrides.erase(handle);
            UpdatePoller(*state); // may stop poller — must NOT hold mutex here
        }
    }

    m_measures.erase(it);
}

double HardwareCore::GetValue(uint32_t handle)
{
    auto it = m_measures.find(handle);
    if (it == m_measures.end())
        return -2.0;

    const MeasureEntry& entry = it->second;
    ModuleState* state = GetModuleState(entry.category);
    if (!state || !state->module)
        return -2.0;

    std::lock_guard<std::mutex> lock(state->mutex);

    if (!state->initialized)
    {
        state->initialized = state->module->Initialize();
        if (state->initialized)
        {
            state->module->GatherAll();
            state->lastGatherTime = GetTimeMs();
        }
    }

    if (!state->initialized)
        return -2.0;

    if (state->currentOverrideMs == 0)
    {
        uint64_t now = GetTimeMs();
        if (now - state->lastGatherTime >= state->minIntervalMs)
        {
            state->module->GatherAll();
            state->lastGatherTime = now;
        }
    }
    // PollerThread is calling GatherAll() independently — just read the cache

    return state->module->GetValue(entry.metricId, entry.deviceIndex);
}

bool HardwareCore::GetString(uint32_t handle, std::wstring& out)
{
    auto it = m_measures.find(handle);
    if (it == m_measures.end())
        return false;

    const MeasureEntry& entry = it->second;
    ModuleState* state = GetModuleState(entry.category);
    if (!state || !state->module)
        return false;

    std::lock_guard<std::mutex> lock(state->mutex);

    if (!state->initialized)
        return false;

    return state->module->GetString(entry.metricId, entry.deviceIndex, out);
}

HardwareCore::ModuleState* HardwareCore::GetModuleState(Category category)
{
    auto it = m_modules.find(category);
    if (it == m_modules.end())
        return nullptr;

    return it->second.get();
}

void HardwareCore::UpdatePoller(ModuleState& state)
{
    if (state.overrides.empty())
    {
        if (state.poller)
        {
            state.poller->Stop(); // blocks until thread exits — mutex must NOT be held
            state.poller.reset();
        }
        state.currentOverrideMs = 0;
        return;
    }

    uint32_t minMs = UINT32_MAX;
    for (auto& [h, ms] : state.overrides)
        minMs = std::min(minMs, ms);

    if (minMs == state.currentOverrideMs)
        return;

    state.currentOverrideMs = minMs;

    if (state.poller)
        state.poller->Stop();
    else
        state.poller = std::make_unique<PollerThread>();

    IModule*    module      = state.module.get();
    std::mutex* mtx         = &state.mutex;
    bool*       initialized = &state.initialized;

    state.poller->Start(minMs, [module, mtx, initialized]()
    {
        std::lock_guard<std::mutex> lock(*mtx);
        if (*initialized)
            module->GatherAll();
    });
}

uint64_t HardwareCore::GetTimeMs() const
{
    return static_cast<uint64_t>(GetTickCount64());
}
