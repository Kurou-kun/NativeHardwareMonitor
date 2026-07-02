#pragma once

#include "Core/IModule.h"
#include "Types/Snapshot.h"

#include <memory>
#include <vector>

#include "Modules/Memory/MemoryResolver.h"

class MemoryModule : public IModule
{
public:
    bool     Initialize() override;
    void     GatherAll() override;
    double   GetValue(uint32_t metricId, uint32_t deviceIndex) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;
    uint32_t GetDeviceCount() const override;

private:
    std::unique_ptr<MemoryResolver> m_resolver;
    std::vector<Snapshot>           m_snapshots;
    bool m_initialized = false;
};
