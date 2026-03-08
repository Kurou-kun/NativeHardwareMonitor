#include "Categories/GPU/NvidiaProvider.h"
#include "Utils/Debug.h"

NvidiaProvider::NvidiaProvider()
{
}

NvidiaProvider::~NvidiaProvider()
{
    m_loader.Shutdown();
}

void NvidiaProvider::LogUnsupported(bool& flag, const wchar_t* msg)
{
    if (!flag)
    {
        LOG_INFO(msg);
        flag = true;
    }
}

bool NvidiaProvider::Initialize()
{
    if (!m_loader.Initialize())
    {
        LOG_ERROR(L"NVML initialization failed");
        return false;
    }

    unsigned int count = 0;

    if (m_loader.DeviceGetCount(&count) != NVML_SUCCESS)
    {
        LOG_ERROR(L"NVML failed to get GPU count");
        return false;
    }

    m_devices.resize(count);

    for (unsigned int i = 0; i < count; i++)
    {
        if (m_loader.DeviceGetHandleByIndex(i, &m_devices[i]) != NVML_SUCCESS)
        {
            LOG_ERROR(L"NVML failed to get device handle");
            return false;
        }
    }

    return count > 0;
}

uint32_t NvidiaProvider::GetDeviceCount() const
{
    return (uint32_t)m_devices.size();
}

bool NvidiaProvider::GetUsage(uint32_t index, double& value)
{
    nvmlUtilization_t util;

    auto result = m_loader.DeviceGetUtilizationRates(
        m_devices[index],
        &util
    );

    if (result == NVML_SUCCESS)
    {
        value = static_cast<double>(util.gpu);
        return true;
    }

    if (result == NVML_ERROR_NOT_SUPPORTED)
        LogUnsupported(logUsage, L"GPU usage metric unsupported");

    return false;
}

bool NvidiaProvider::GetTemperature(uint32_t index, double& value)
{
    unsigned int temp = 0;

    auto r = m_loader.DeviceGetTemperature(
        m_devices[index],
        NVML_TEMPERATURE_GPU,
        &temp
    );

    if (r == NVML_SUCCESS)
    {
        value = (double)temp;
        return true;
    }

    return false;
}
bool NvidiaProvider::GetVramUsed(uint32_t index, uint64_t& value)
{
    nvmlMemory_t mem{};

    auto result = m_loader.DeviceGetMemoryInfo(
        m_devices[index],
        &mem
    );

    if (result == NVML_SUCCESS)
    {
        value = mem.used;
        return true;
    }

    if (result == NVML_ERROR_NOT_SUPPORTED)
    {
        if (!logVramUsed)
        {
            LOG_INFO(L"GPU VRAM used metric unsupported");
            logVramUsed = true;
        }
    }

    return false;
}

bool NvidiaProvider::GetVramTotal(uint32_t index, uint64_t& value)
{
    nvmlMemory_t mem{};

    auto result = m_loader.DeviceGetMemoryInfo(
        m_devices[index],
        &mem
    );

    if (result == NVML_SUCCESS)
    {
        value = mem.total;
        return true;
    }

    if (result == NVML_ERROR_NOT_SUPPORTED)
    {
        if (!logVramTotal)
        {
            LOG_INFO(L"GPU VRAM total metric unsupported");
            logVramTotal = true;
        }
    }

    return false;
}

bool NvidiaProvider::GetCoreClock(uint32_t index, double& value)
{
    unsigned int clock;

    auto result = m_loader.DeviceGetClockInfo(
        m_devices[index],
        NVML_CLOCK_GRAPHICS,
        &clock
    );

    if (result == NVML_SUCCESS)
    {
        // NVML returns MHz -> convert to Hz
        value = static_cast<double>(clock) * 1000000.0;
        return true;
    }

    if (result == NVML_ERROR_NOT_SUPPORTED)
    {
        if (!logCoreClock)
        {
            LOG_INFO(L"GPU core clock metric unsupported");
            logCoreClock = true;
        }
    }

    return false;
}

bool NvidiaProvider::GetMemoryClock(uint32_t index, double& value)
{
    unsigned int clock;

    auto result = m_loader.DeviceGetClockInfo(
        m_devices[index],
        NVML_CLOCK_MEM,
        &clock
    );

    if (result == NVML_SUCCESS)
    {
        // NVML returns MHz -> convert to Hz
        value = static_cast<double>(clock) * 1000000.0;
        return true;
    }

    if (result == NVML_ERROR_NOT_SUPPORTED)
    {
        if (!logMemClock)
        {
            LOG_INFO(L"GPU memory clock metric unsupported");
            logMemClock = true;
        }
    }

    return false;
}

bool NvidiaProvider::GetFanSpeed(uint32_t index, double& value)
{
    unsigned int speed;

    auto result = m_loader.DeviceGetFanSpeed(
        m_devices[index],
        &speed
    );

    if (result == NVML_SUCCESS)
    {
        value = static_cast<double>(speed);
        return true;
    }

    if (result == NVML_ERROR_NOT_SUPPORTED)
        LogUnsupported(logFan, L"GPU fan speed metric unsupported");

    return false;
}

bool NvidiaProvider::GetPower(uint32_t index, double& value)
{
    unsigned int power;

    auto result = m_loader.DeviceGetPowerUsage(
        m_devices[index],
        &power
    );

    if (result == NVML_SUCCESS)
    {
        value = static_cast<double>(power) / 1000.0;
        return true;
    }

    if (result == NVML_ERROR_NOT_SUPPORTED)
        LogUnsupported(logPowerUsage, L"GPU power usage metric unsupported");

    return false;
}

bool NvidiaProvider::GetPowerLimit(uint32_t index, double& value)
{
    unsigned int power;

    auto result = m_loader.DeviceGetPowerLimit(
        m_devices[index],
        &power
    );

    if (result == NVML_SUCCESS)
    {
        value = static_cast<double>(power) / 1000.0;
        return true;
    }

    if (result == NVML_ERROR_NOT_SUPPORTED)
        LogUnsupported(logPowerLimit, L"GPU power limit metric unsupported");

    return false;
}

bool NvidiaProvider::GetFanSpeedRPM(uint32_t index, double& value)
{
    if (!logFanRPM)
    {
        LOG_INFO(L"GPU fan RPM metric unsupported");
        logFanRPM = true;
    }

    return false;
}

bool NvidiaProvider::GetMemoryTemperature(uint32_t index, double& value)
{
    if (!logMemTemp)
    {
        LOG_INFO(L"GPU memory temperature metric unsupported");
        logMemTemp = true;
    }

    return false;
}

bool NvidiaProvider::GetHotspotTemperature(uint32_t index, double& value)
{
    if (!logHotspot)
    {
        LOG_INFO(L"GPU hotspot temperature metric unsupported");
        logHotspot = true;
    }

    return false;
}