#include "Core/HardwareCore.h"

#include "Categories/Network/NetworkBackend.h"
#include "Categories/Memory/MemoryBackend.h"
#include "Categories/CPU/CpuBackend.h"
#include "Categories/Storage/StorageBackend.h"
#include "Categories/Dummy/DummyBackend.h"

#include "Categories/GPU/GpuBackend.h"

#include <windows.h>

HardwareCore& HardwareCore::Instance()
{
    static HardwareCore instance;
    return instance;
}

HardwareCore::HardwareCore()
{
    m_backends[Category::GPU].backend = std::make_unique<GpuBackend>();
    m_backends[Category::CPU].backend = std::make_unique<CpuBackend>();
    m_backends[Category::Memory].backend = std::make_unique<MemoryBackend>();
    m_backends[Category::Network].backend = std::make_unique<NetworkBackend>();
    m_backends[Category::Storage].backend = std::make_unique<StorageBackend>();
}

uint32_t HardwareCore::RegisterMeasure(
    Category category,
    uint32_t metricId,
    uint32_t deviceIndex)
{
    uint32_t handle = m_nextHandle++;

    m_measures.emplace(handle, MeasureEntry{
        category,
        metricId,
        deviceIndex
        });

    return handle;
}

void HardwareCore::UnregisterMeasure(uint32_t handle)
{
    m_measures.erase(handle);
}

double HardwareCore::GetValue(uint32_t handle)
{
    auto it = m_measures.find(handle);
    if (it == m_measures.end())
        return 0.0;

    const MeasureEntry& entry = it->second;

    BackendState* state = GetBackendState(entry.category);
    if (!state || !state->backend)
        return 0.0;

    if (!state->backend->Initialize())
        return 0.0;

    uint64_t now = GetTimeMs();

    if (now - state->lastUpdateTime >= state->minIntervalMs)
    {
        state->backend->Update();
        state->lastUpdateTime = now;
    }

    return state->backend->GetValue(entry.deviceIndex, entry.metricId);
}

HardwareCore::BackendState* HardwareCore::GetBackendState(Category category)
{
    auto it = m_backends.find(category);
    if (it == m_backends.end())
        return nullptr;

    return &it->second;
}

uint64_t HardwareCore::GetTimeMs() const
{
    return static_cast<uint64_t>(GetTickCount64());
}