#pragma once

#include <cstdint>

class IMemoryProvider
{
public:

    virtual ~IMemoryProvider() = default;

    virtual bool Initialize() = 0;

    virtual void Update() = 0;

    virtual bool GetUsed(uint32_t deviceIndex, double& value) = 0;

    virtual bool GetFree(uint32_t deviceIndex, double& value) = 0;

    virtual bool GetTotal(uint32_t deviceIndex, double& value) = 0;

    virtual bool GetUsedPercent(uint32_t deviceIndex, double& value) = 0;

    virtual uint32_t GetDeviceCount() const = 0;
};