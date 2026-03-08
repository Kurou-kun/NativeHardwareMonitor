#include "Categories/Memory/WinApiMemoryProvider.h"
#include "Utils/Debug.h"

bool WinApiMemoryProvider::Initialize()
{
    m_memory.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&m_memory))
    {
        LOG_ERROR(L"GlobalMemoryStatusEx initialization failed");
        return false;
    }

    Update();

    LOG_INFO(L"WinApiMemoryProvider initialized");

    return true;
}

void WinApiMemoryProvider::Update()
{
    if (!GlobalMemoryStatusEx(&m_memory))
    {
        LOG_ERROR(L"GlobalMemoryStatusEx update failed");
        return;
    }

    m_ramTotal = m_memory.ullTotalPhys;
    m_ramFree = m_memory.ullAvailPhys;

    m_swapTotal = m_memory.ullTotalPageFile;
    m_swapFree = m_memory.ullAvailPageFile;
}

bool WinApiMemoryProvider::GetTotal(uint32_t deviceIndex, double& value)
{
    if (deviceIndex == 0)
        value = static_cast<double>(m_ramTotal);
    else if (deviceIndex == 1)
        value = static_cast<double>(m_swapTotal);
    else
        return false;

    return true;
}

bool WinApiMemoryProvider::GetFree(uint32_t deviceIndex, double& value)
{
    if (deviceIndex == 0)
        value = static_cast<double>(m_ramFree);
    else if (deviceIndex == 1)
        value = static_cast<double>(m_swapFree);
    else
        return false;

    return true;
}

bool WinApiMemoryProvider::GetUsed(uint32_t deviceIndex, double& value)
{
    uint64_t total = 0;
    uint64_t free = 0;

    if (deviceIndex == 0)
    {
        total = m_ramTotal;
        free = m_ramFree;
    }
    else if (deviceIndex == 1)
    {
        total = m_swapTotal;
        free = m_swapFree;
    }
    else
        return false;

    value = static_cast<double>(total - free);

    return true;
}

bool WinApiMemoryProvider::GetUsedPercent(uint32_t deviceIndex, double& value)
{
    uint64_t total = 0;
    uint64_t free = 0;

    if (deviceIndex == 0)
    {
        total = m_ramTotal;
        free = m_ramFree;
    }
    else if (deviceIndex == 1)
    {
        total = m_swapTotal;
        free = m_swapFree;
    }
    else
        return false;

    if (total == 0)
        return false;

    value = (static_cast<double>(total - free) / static_cast<double>(total)) * 100.0;

    return true;
}

uint32_t WinApiMemoryProvider::GetDeviceCount() const
{
    return 2; // RAM + Pagefile
}