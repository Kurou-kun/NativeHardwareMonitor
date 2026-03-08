#pragma once

#include <stdint.h>

class MsrDriver
{
public:

    static bool Initialize();
    static void Shutdown();

    static bool IsAvailable();

    static bool Read(uint32_t msr, uint64_t& value);
    static bool ReadCore(uint32_t core, uint32_t msr, uint64_t& value);

};