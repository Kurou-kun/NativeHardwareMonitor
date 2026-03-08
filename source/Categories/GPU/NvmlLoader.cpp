#include "Categories/GPU/NvmlLoader.h"
#include "Utils/Debug.h"

bool NvmlLoader::Initialize()
{
    m_module = LoadLibraryW(L"nvml.dll");

    if (!m_module)
    {
        LOG_ERROR(L"NVML: failed to load nvml.dll");
        return false;
    }

    LOG_INFO(L"NVML: nvml.dll loaded");

    m_nvmlInit_v2 =
        (decltype(m_nvmlInit_v2))GetProcAddress(m_module, "nvmlInit_v2");

    m_nvmlShutdown =
        (decltype(m_nvmlShutdown))GetProcAddress(m_module, "nvmlShutdown");

    m_nvmlDeviceGetCount_v2 =
        (decltype(m_nvmlDeviceGetCount_v2))GetProcAddress(m_module, "nvmlDeviceGetCount_v2");

    m_nvmlDeviceGetHandleByIndex_v2 =
        (decltype(m_nvmlDeviceGetHandleByIndex_v2))GetProcAddress(m_module, "nvmlDeviceGetHandleByIndex_v2");

    m_nvmlDeviceGetUtilizationRates =
        (decltype(m_nvmlDeviceGetUtilizationRates))GetProcAddress(m_module, "nvmlDeviceGetUtilizationRates");

    m_nvmlDeviceGetMemoryInfo =
        (decltype(m_nvmlDeviceGetMemoryInfo))GetProcAddress(m_module, "nvmlDeviceGetMemoryInfo");

    m_nvmlDeviceGetTemperature =
        (decltype(m_nvmlDeviceGetTemperature))GetProcAddress(m_module, "nvmlDeviceGetTemperature");

    m_nvmlDeviceGetClockInfo =
        (decltype(m_nvmlDeviceGetClockInfo))GetProcAddress(m_module, "nvmlDeviceGetClockInfo");

    m_nvmlDeviceGetFanSpeed =
        (decltype(m_nvmlDeviceGetFanSpeed))GetProcAddress(m_module, "nvmlDeviceGetFanSpeed");

    m_nvmlDeviceGetPowerUsage =
        (decltype(m_nvmlDeviceGetPowerUsage))GetProcAddress(m_module, "nvmlDeviceGetPowerUsage");

    m_nvmlDeviceGetPowerManagementLimit =
        (decltype(m_nvmlDeviceGetPowerManagementLimit))GetProcAddress(m_module, "nvmlDeviceGetPowerManagementLimit");

    if (!m_nvmlInit_v2 ||
        !m_nvmlDeviceGetCount_v2 ||
        !m_nvmlDeviceGetHandleByIndex_v2)
    {
        LOG_ERROR(L"NVML: required functions missing");
        return false;
    }

    nvmlReturn_t r = m_nvmlInit_v2();

    if (r != NVML_SUCCESS)
    {
        LOG_ERROR(L"NVML: nvmlInit_v2 failed (%d)", r);
        return false;
    }

    LOG_INFO(L"NVML initialized successfully");

    unsigned int count = 0;

    if (DeviceGetCount(&count) == NVML_SUCCESS)
    {
        LOG_INFO(L"NVML: detected %u GPU(s)", count);
    }

    return true;
}

void NvmlLoader::Shutdown()
{
    if (m_nvmlShutdown)
    {
        m_nvmlShutdown();
        LOG_INFO(L"NVML shutdown");
    }

    if (m_module)
    {
        FreeLibrary(m_module);
        m_module = nullptr;
    }
}

nvmlReturn_t NvmlLoader::DeviceGetCount(unsigned int* count)
{
    if (!m_nvmlDeviceGetCount_v2)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetCount_v2(count);
}

nvmlReturn_t NvmlLoader::DeviceGetHandleByIndex(unsigned int index, nvmlDevice_t* device)
{
    if (!m_nvmlDeviceGetHandleByIndex_v2)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetHandleByIndex_v2(index, device);
}

nvmlReturn_t NvmlLoader::DeviceGetUtilizationRates(nvmlDevice_t device, nvmlUtilization_t* util)
{
    if (!m_nvmlDeviceGetUtilizationRates)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetUtilizationRates(device, util);
}

nvmlReturn_t NvmlLoader::DeviceGetMemoryInfo(nvmlDevice_t device, nvmlMemory_t* mem)
{
    if (!m_nvmlDeviceGetMemoryInfo)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetMemoryInfo(device, mem);
}

nvmlReturn_t NvmlLoader::DeviceGetTemperature(nvmlDevice_t device, nvmlTemperatureSensors_t sensor, unsigned int* temp)
{
    if (!m_nvmlDeviceGetTemperature)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetTemperature(device, sensor, temp);
}

nvmlReturn_t NvmlLoader::DeviceGetClockInfo(nvmlDevice_t device, nvmlClockType_t type, unsigned int* clock)
{
    if (!m_nvmlDeviceGetClockInfo)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetClockInfo(device, type, clock);
}

nvmlReturn_t NvmlLoader::DeviceGetFanSpeed(nvmlDevice_t device, unsigned int* speed)
{
    if (!m_nvmlDeviceGetFanSpeed)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetFanSpeed(device, speed);
}

nvmlReturn_t NvmlLoader::DeviceGetPowerUsage(nvmlDevice_t device, unsigned int* power)
{
    if (!m_nvmlDeviceGetPowerUsage)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetPowerUsage(device, power);
}

nvmlReturn_t NvmlLoader::DeviceGetPowerLimit(nvmlDevice_t device, unsigned int* limit)
{
    if (!m_nvmlDeviceGetPowerManagementLimit)
        return NVML_ERROR_FUNCTION_NOT_FOUND;

    return m_nvmlDeviceGetPowerManagementLimit(device, limit);
}