#pragma once

#include <windows.h>
#include <nvml.h>

class NvidiaLoader
{
public:
    bool Initialize();
    void Shutdown();

    bool IsLoaded() const { return m_module != nullptr; }

    // NVML wrappers
    nvmlReturn_t DeviceGetCount(unsigned int* count);
    nvmlReturn_t DeviceGetHandleByIndex(unsigned int index, nvmlDevice_t* device);
    nvmlReturn_t DeviceGetUtilizationRates(nvmlDevice_t device, nvmlUtilization_t* util);
    nvmlReturn_t DeviceGetMemoryInfo(nvmlDevice_t device, nvmlMemory_t* mem);

private:
    HMODULE m_module = nullptr;

    // function pointers
    nvmlReturn_t(*m_nvmlInit_v2)() = nullptr;
    nvmlReturn_t(*m_nvmlShutdown)() = nullptr;
    nvmlReturn_t(*m_nvmlDeviceGetCount_v2)(unsigned int*) = nullptr;
    nvmlReturn_t(*m_nvmlDeviceGetHandleByIndex_v2)(unsigned int, nvmlDevice_t*) = nullptr;
    nvmlReturn_t(*m_nvmlDeviceGetUtilizationRates)(nvmlDevice_t, nvmlUtilization_t*) = nullptr;
    nvmlReturn_t(*m_nvmlDeviceGetMemoryInfo)(nvmlDevice_t, nvmlMemory_t*) = nullptr;
};