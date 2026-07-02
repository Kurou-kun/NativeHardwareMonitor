#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <vector>

class Adl2Provider : public IProvider
{
public:
    ~Adl2Provider() { Shutdown(); }

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    void Shutdown();
    bool LoadFunctions();
    void EnumerateAdapters();

    HMODULE m_module = nullptr;
    void*   m_ctx    = nullptr; // ADL_CONTEXT_HANDLE

    std::vector<int> m_adapterIndices; // active AMD adapter indices, one per physical GPU

    // Function pointers (typed in .cpp via ADL headers; kept as void* here)
    void* m_Create      = nullptr;
    void* m_Destroy     = nullptr;
    void* m_NumAdapters = nullptr;
    void* m_AdapterInfo = nullptr;
    void* m_Active      = nullptr;
    void* m_Activity    = nullptr;
    void* m_Temperature = nullptr;
    void* m_FanSpeed    = nullptr;
};
