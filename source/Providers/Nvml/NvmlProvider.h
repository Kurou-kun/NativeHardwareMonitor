#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <vector>
#include <nvml.h>

class NvmlProvider : public IProvider
{
public:
    ~NvmlProvider() { Shutdown(); }

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    HMODULE                  m_module  = nullptr;
    std::vector<nvmlDevice_t> m_devices;

    bool LoadFunctions();
    void Shutdown();

    decltype(&nvmlInit_v2)                       m_Init            = nullptr;
    decltype(&nvmlShutdown)                      m_Shutdown        = nullptr;
    decltype(&nvmlDeviceGetCount_v2)             m_GetCount        = nullptr;
    decltype(&nvmlDeviceGetHandleByIndex_v2)     m_GetHandle       = nullptr;
    decltype(&nvmlDeviceGetUtilizationRates)     m_GetUtil         = nullptr;
    decltype(&nvmlDeviceGetMemoryInfo)           m_GetMemory       = nullptr;
    decltype(&nvmlDeviceGetTemperature)          m_GetTemp         = nullptr;
    decltype(&nvmlDeviceGetClockInfo)            m_GetClock        = nullptr;
    decltype(&nvmlDeviceGetFanSpeed)             m_GetFan          = nullptr;
    decltype(&nvmlDeviceGetPowerUsage)           m_GetPower        = nullptr;
    decltype(&nvmlDeviceGetPowerManagementLimit) m_GetPowerLimit   = nullptr;
};
