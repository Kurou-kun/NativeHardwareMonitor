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

    // Optional — absent on older drivers/architectures; must not fail Initialize()
    decltype(&nvmlDeviceGetFanSpeedRPM)          m_GetFanRPM       = nullptr;
    decltype(&nvmlDeviceGetFieldValues)          m_GetFieldValues  = nullptr;
    decltype(&nvmlDeviceGetMaxClockInfo)         m_GetMaxClock     = nullptr;
    decltype(&nvmlDeviceGetCurrPcieLinkGeneration) m_GetPcieGen    = nullptr;
    decltype(&nvmlDeviceGetCurrPcieLinkWidth)    m_GetPcieWidth    = nullptr;
    decltype(&nvmlDeviceGetEncoderUtilization)   m_GetEncoderUtil  = nullptr;
    decltype(&nvmlDeviceGetDecoderUtilization)   m_GetDecoderUtil  = nullptr;
    decltype(&nvmlDeviceGetPerformanceState)     m_GetPState       = nullptr;
    // Deprecated in NVML 13 but still exported; manual typedef avoids the decltype deprecation warning.
    typedef nvmlReturn_t (*GetThrottle_t)(nvmlDevice_t, unsigned long long*);
    GetThrottle_t                                m_GetThrottle     = nullptr;

    // String info — optional, absent on older drivers; must not fail Initialize()
    decltype(&nvmlDeviceGetName)                 m_GetName         = nullptr;
    decltype(&nvmlSystemGetDriverVersion)        m_GetDriverVersion= nullptr;
    decltype(&nvmlDeviceGetVbiosVersion)         m_GetVbios        = nullptr;
    decltype(&nvmlDeviceGetPciInfo_v3)           m_GetPciInfo      = nullptr;
};
