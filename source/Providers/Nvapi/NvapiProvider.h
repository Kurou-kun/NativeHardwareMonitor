#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>

class NvapiProvider : public IProvider
{
public:
    ~NvapiProvider() { Shutdown(); }

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    void Shutdown();
    bool LoadFunctions();

    HMODULE m_module = nullptr;

    // nvapi_QueryInterface resolves all symbols at runtime — no nvapi64.lib needed
    void* (__cdecl* m_QueryInterface)(unsigned int) = nullptr;

    // Raw function pointers (typed in .cpp via NVAPI headers; kept as void* here)
    void* m_Init      = nullptr;
    void* m_Unload    = nullptr;
    void* m_EnumGPUs  = nullptr;
    void* m_GetTherm  = nullptr;
    void* m_GetClocks = nullptr;
    void* m_GetPstates= nullptr;
    void* m_GetMemory = nullptr;
    void* m_GetTach   = nullptr; // optional — not present on fanless GPUs/older drivers

    void*    m_gpus[64] = {};
    uint32_t m_count    = 0;
};
