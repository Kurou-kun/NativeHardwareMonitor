#pragma once

#include <cstdint>
#include <string>

#include "Types/Snapshot.h"

class IProvider
{
public:
    virtual ~IProvider() = default;

    virtual bool     Initialize() = 0;
    virtual uint32_t GetDeviceCount() const = 0;
    virtual void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) = 0;
    virtual bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) = 0;

    // Ping only: resolves a host string to a stable device slot. Every other provider
    // inherits this no-op, which is cheaper than fifteen empty overrides.
    virtual uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) { return 0; }
};
