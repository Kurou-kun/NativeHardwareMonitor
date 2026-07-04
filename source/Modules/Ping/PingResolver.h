#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <memory>
#include <string>

class PingResolver
{
public:
    bool     Initialize();
    uint32_t GetDeviceCount() const;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap);
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out);
    uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs);

private:
    std::unique_ptr<IProvider> m_provider;
};
