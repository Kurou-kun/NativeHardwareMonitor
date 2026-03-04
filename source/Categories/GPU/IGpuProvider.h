#pragma once

#include <cstdint>

class IGpuProvider
{
public:
    virtual ~IGpuProvider() = default;

    virtual bool Initialize() = 0;
    virtual uint32_t GetDeviceCount() const = 0;

    virtual bool GetUtilization(uint32_t index, double& value) = 0;
    virtual bool GetMemoryUsed(uint32_t index, uint64_t& value) = 0;
};