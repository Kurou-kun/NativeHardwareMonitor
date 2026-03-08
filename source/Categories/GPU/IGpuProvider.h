#pragma once

#include <cstdint>

class IGpuProvider
{
public:

    virtual ~IGpuProvider() = default;

    virtual bool Initialize() = 0;
    virtual uint32_t GetDeviceCount() const = 0;

    virtual bool GetUsage(uint32_t index, double& value) { return false; }

    virtual bool GetVramUsed(uint32_t index, uint64_t& value) { return false; }
    virtual bool GetVramTotal(uint32_t index, uint64_t& value) { return false; }

    virtual bool GetTemperature(uint32_t index, double& value) { return false; }
    virtual bool GetHotspotTemperature(uint32_t index, double& value) { return false; }
    virtual bool GetMemoryTemperature(uint32_t index, double& value) { return false; }

    virtual bool GetCoreClock(uint32_t index, double& value) { return false; }
    virtual bool GetMemoryClock(uint32_t index, double& value) { return false; }

    virtual bool GetFanSpeed(uint32_t index, double& value) { return false; }
    virtual bool GetFanSpeedRPM(uint32_t index, double& value) { return false; }

    virtual bool GetPower(uint32_t index, double& value) { return false; }
    virtual bool GetPowerLimit(uint32_t index, double& value) { return false; }
};