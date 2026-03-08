#pragma once

#include <vector>
#include <stdint.h>

#include "Categories/GPU/IGpuProvider.h"
#include "Categories/GPU/NvmlLoader.h"

class NvidiaProvider : public IGpuProvider
{
public:

    NvidiaProvider();
    ~NvidiaProvider();

    bool Initialize() override;

    uint32_t GetDeviceCount() const override;

    bool GetUsage(uint32_t index, double& value) override;

    bool GetVramUsed(uint32_t index, uint64_t& value) override;
    bool GetVramTotal(uint32_t index, uint64_t& value) override;

    bool GetTemperature(uint32_t index, double& value) override;
    bool GetHotspotTemperature(uint32_t index, double& value) override;
    bool GetMemoryTemperature(uint32_t index, double& value) override;

    bool GetCoreClock(uint32_t index, double& value) override;
    bool GetMemoryClock(uint32_t index, double& value) override;

    bool GetFanSpeed(uint32_t index, double& value) override;
    bool GetFanSpeedRPM(uint32_t index, double& value) override;

    bool GetPower(uint32_t index, double& value) override;
    bool GetPowerLimit(uint32_t index, double& value) override;

    void LogUnsupported(bool& flag, const wchar_t* msg);

private:

    NvmlLoader m_loader;
    std::vector<nvmlDevice_t> m_devices;

    bool logUsage = false;
    bool logTemp = false;

    bool logVramUsed = false;
    bool logVramTotal = false;

    bool logCoreClock = false;
    bool logMemClock = false;

    bool logFan = false;

    bool logPowerUsage = false;
    bool logPowerLimit = false;

    bool logFanRPM = false;
    bool logMemTemp = false;
    bool logHotspot = false;
};