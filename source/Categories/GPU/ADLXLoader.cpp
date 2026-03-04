#include "ADLXLoader.h"

bool ADLXLoader::Initialize()
{
    if (m_initialized)
        return true;

    if (g_ADLX.Initialize() != ADLX_OK)
        return false;

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

    return g_ADLX.GetSystemServices();
}