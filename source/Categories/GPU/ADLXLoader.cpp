#include "ADLXLoader.h"

bool ADLXLoader::Initialize()
{
    if (m_initialized)
        return true;

    if (g_ADLX.Initialize() != ADLX_OK)
        return false;

    auto system = g_ADLX.GetSystemServices();

    if (!system)
    {
        g_ADLX.Terminate();
        return false;
    }

    m_initialized = true;
    return true;
}


void ADLXLoader::Shutdown()
{
    if (!m_initialized)
        return;

    g_ADLX.Terminate();
    m_initialized = false;
}

adlx::IADLXSystem* ADLXLoader::GetSystemServices() const
{
    if (!m_initialized)
        return nullptr;

    auto system = g_ADLX.GetSystemServices();

    if (!system)
        return nullptr;

    return system;
}