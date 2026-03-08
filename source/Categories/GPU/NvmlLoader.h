#pragma once

#include <windows.h>
#include <nvml.h>

class NvmlLoader
{
public:

    bool Initialize();
    void Shutdown();

    nvmlReturn_t DeviceGetCount(unsigned int* count);
    nvmlReturn_t DeviceGetHandleByIndex(unsigned int index, nvmlDevice_t* device);

    nvmlReturn_t DeviceGetUtilizationRates(nvmlDevice_t device, nvmlUtilization_t* util);
    nvmlReturn_t DeviceGetMemoryInfo(nvmlDevice_t device, nvmlMemory_t* mem);

    nvmlReturn_t DeviceGetTemperature(nvmlDevice_t device, nvmlTemperatureSensors_t sensor, unsigned int* temp);

    nvmlReturn_t DeviceGetClockInfo(nvmlDevice_t device, nvmlClockType_t type, unsigned int* clock);

    nvmlReturn_t DeviceGetFanSpeed(nvmlDevice_t device, unsigned int* speed);

    nvmlReturn_t DeviceGetPowerUsage(nvmlDevice_t device, unsigned int* power);
    nvmlReturn_t DeviceGetPowerLimit(nvmlDevice_t device, unsigned int* limit);

private:

    HMODULE m_module = nullptr;

    decltype(&nvmlInit_v2) m_nvmlInit_v2 = nullptr;
    decltype(&nvmlShutdown) m_nvmlShutdown = nullptr;

    decltype(&nvmlDeviceGetCount_v2) m_nvmlDeviceGetCount_v2 = nullptr;
    decltype(&nvmlDeviceGetHandleByIndex_v2) m_nvmlDeviceGetHandleByIndex_v2 = nullptr;

    decltype(&nvmlDeviceGetUtilizationRates) m_nvmlDeviceGetUtilizationRates = nullptr;
    decltype(&nvmlDeviceGetMemoryInfo) m_nvmlDeviceGetMemoryInfo = nullptr;

    decltype(&nvmlDeviceGetTemperature) m_nvmlDeviceGetTemperature = nullptr;

    decltype(&nvmlDeviceGetClockInfo) m_nvmlDeviceGetClockInfo = nullptr;

    decltype(&nvmlDeviceGetFanSpeed) m_nvmlDeviceGetFanSpeed = nullptr;

    decltype(&nvmlDeviceGetPowerUsage) m_nvmlDeviceGetPowerUsage = nullptr;
    decltype(&nvmlDeviceGetPowerManagementLimit) m_nvmlDeviceGetPowerManagementLimit = nullptr;
};