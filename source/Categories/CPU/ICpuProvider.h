#pragma once

#include <cstdint>

class ICpuProvider
{
public:

    virtual ~ICpuProvider() = default;

    virtual bool Initialize() = 0;
    virtual void Update() = 0;

    virtual bool GetTotalUsage(double& value) = 0;
    virtual bool GetCoreUsage(uint32_t coreIndex, double& value) = 0;

    virtual bool GetClock(double& value) = 0;

    virtual uint32_t GetCoreCount() const = 0;
};