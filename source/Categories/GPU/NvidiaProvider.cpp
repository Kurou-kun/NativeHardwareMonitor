#include "Categories/GPU/NvidiaProvider.h"

bool NvidiaProvider::Initialize()
{
    if (!m_loader.Initialize())
        return false;

    unsigned int count = 0;
    if (m_loader.DeviceGetCount(&count) != NVML_SUCCESS)
        return false;

    m_deviceCount = count;
    return true;
}

uint32_t NvidiaProvider::GetDeviceCount() const
{
    return m_deviceCount;
}

bool NvidiaProvider::GetUtilization(uint32_t index, double& value)
{
    if (index >= m_deviceCount)
        return false;

    nvmlDevice_t device;
    if (m_loader.DeviceGetHandleByIndex(index, &device) != NVML_SUCCESS)
        return false;

    nvmlUtilization_t util{};
    if (m_loader.DeviceGetUtilizationRates(device, &util) != NVML_SUCCESS)
        return false;

    value = static_cast<double>(util.gpu);
    return true;
}

bool NvidiaProvider::GetMemoryUsed(uint32_t index, uint64_t& value)
{
    if (index >= m_deviceCount)
        return false;

    nvmlDevice_t device;
    if (m_loader.DeviceGetHandleByIndex(index, &device) != NVML_SUCCESS)
        return false;

    nvmlMemory_t mem{};
    if (m_loader.DeviceGetMemoryInfo(device, &mem) != NVML_SUCCESS)
        return false;

    value = mem.used;
    return true;
}