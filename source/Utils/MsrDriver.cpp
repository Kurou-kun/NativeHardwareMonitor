#include "Utils/MsrDriver.h"

bool MsrDriver::Open()
{
    // TODO: open kernel MSR driver handle
    return false;
}

void MsrDriver::Close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool MsrDriver::ReadMsr(uint32_t index, uint64_t& value)
{
    // TODO: implement MSR read via driver
    return false;
}
