#include "Categories/GPU/NvidiaLoader.h"

bool NvidiaLoader::Initialize()
{
    m_module = LoadLibraryW(L"nvml.dll");
    if (!m_module)
        return false;

    m_nvmlInit_v2 = (decltype(m_nvmlInit_v2))GetProcAddress(m_module, "nvmlInit_v2");
    m_nvmlShutdown = (decltype(m_nvmlShutdown))GetProcAddress(m_module, "nvmlShutdown");
    m_nvmlDeviceGetCount_v2 = (decltype(m_nvmlDeviceGetCount_v2))GetProcAddress(m_module, "nvmlDeviceGetCount_v2");
    m_nvmlDeviceGetHandleByIndex_v2 = (decltype(m_nvmlDeviceGetHandleByIndex_v2))GetProcAddress(m_module, "nvmlDeviceGetHandleByIndex_v2");
    m_nvmlDeviceGetUtilizationRates = (decltype(m_nvmlDeviceGetUtilizationRates))GetProcAddress(m_module, "nvmlDeviceGetUtilizationRates");
    m_nvmlDeviceGetMemoryInfo = (decltype(m_nvmlDeviceGetMemoryInfo))GetProcAddress(m_module, "nvmlDeviceGetMemoryInfo");

    if (!m_nvmlInit_v2 || !m_nvmlDeviceGetCount_v2)
        return false;

    return m_nvmlInit_v2() == NVML_SUCCESS;
}

void NvidiaLoader::Shutdown()
{
    if (m_nvmlShutdown)
        m_nvmlShutdown();

    if (m_module)
        FreeLibrary(m_module);

    m_module = nullptr;
}

nvmlReturn_t NvidiaLoader::DeviceGetCount(unsigned int* count)
{
    return m_nvmlDeviceGetCount_v2(count);
}

nvmlReturn_t NvidiaLoader::DeviceGetHandleByIndex(unsigned int index, nvmlDevice_t* device)
{
    return m_nvmlDeviceGetHandleByIndex_v2(index, device);
}

nvmlReturn_t NvidiaLoader::DeviceGetUtilizationRates(nvmlDevice_t device, nvmlUtilization_t* util)
{
    return m_nvmlDeviceGetUtilizationRates(device, util);
}

nvmlReturn_t NvidiaLoader::DeviceGetMemoryInfo(nvmlDevice_t device, nvmlMemory_t* mem)
{
    return m_nvmlDeviceGetMemoryInfo(device, mem);
}