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

    void CacheAdapterStrings(int adapterIndex, const char* pnpString);
    void GatherPMLog(uint32_t deviceIndex, int adapterIndex, Snapshot& snap);

    HMODULE m_module = nullptr;
    void*   m_ctx    = nullptr; // ADL_CONTEXT_HANDLE

    std::vector<int>         m_adapterIndices; // active AMD adapter indices, one per physical GPU
    std::vector<std::wstring> m_adapterNames;   // cached from AdapterInfo.strAdapterName, same order as m_adapterIndices
    std::vector<std::wstring> m_vbiosVersions;  // from VideoBiosInfo_Get, empty where unsupported
    std::vector<std::wstring> m_pciIds;         // parsed out of AdapterInfo.strPNPString

    // Last PMLog throttler bitmask per device, -1 where the sensor is absent. Written by
    // the poller in GatherSnapshot and read by GetString on Rainmeter's thread; both run
    // under the module mutex (HardwareCore), so no separate lock.
    std::vector<int> m_throttleStatus;

    // Function pointers (typed in .cpp via ADL headers; kept as void* here)
    void* m_Create      = nullptr;
    void* m_Destroy     = nullptr;
    void* m_NumAdapters = nullptr;
    void* m_AdapterInfo = nullptr;
    void* m_Activity    = nullptr;
    void* m_Temperature = nullptr;
    void* m_FanSpeed    = nullptr;

    // Optional — missing on older drivers, so a null here is normal and not an error.
    void* m_PMLog       = nullptr;
    void* m_VramUsage   = nullptr;
    void* m_MemoryInfo2 = nullptr;
    void* m_BiosInfo    = nullptr;
};
