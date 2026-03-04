#pragma once

#include "ADLXHelper.h"

class ADLXLoader
{
public:
    bool Initialize();
    void Shutdown();

    adlx::IADLXSystem* GetSystemServices() const;

private:
    bool m_initialized = false;
};