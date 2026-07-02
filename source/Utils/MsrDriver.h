#pragma once

#include <cstdint>
#include <windows.h>

class MsrDriver
{
public:
    bool Open();
    void Close();

    bool ReadMsr(uint32_t index, uint64_t& value);

private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;
};
