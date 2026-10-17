#include "Providers/WinApi/WinApiGpuProvider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"
#include "Utils/PdhUtil.h"

#include <dxgi1_4.h>

#include <algorithm>
#include <cwctype>
#include <map>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "dxgi.lib")

bool WinApiGpuProvider::Initialize()
{
    if (!EnumerateAdapters())
    {
        LOG_STARTUP(L"WinApiGpuProvider: no DXGI adapter enumerated");
        return false;
    }

    // The GPU counter set arrived with Windows 10 1709 and needs a WDDM 2.x driver, so
    // treat its absence as "no usage metrics", not as a failure: DXGI still gives the
    // memory totals, which is more than a missing vendor SDK gives.
    if (PdhOpenQueryW(nullptr, 0, &m_query) != ERROR_SUCCESS)
    {
        m_query = nullptr;
    }
    else if (PdhAddEnglishCounterW(m_query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &m_engine) != ERROR_SUCCESS ||
             PdhAddEnglishCounterW(m_query, L"\\GPU Adapter Memory(*)\\Dedicated Usage", 0, &m_dedicated) != ERROR_SUCCESS ||
             PdhAddEnglishCounterW(m_query, L"\\GPU Adapter Memory(*)\\Shared Usage", 0, &m_shared) != ERROR_SUCCESS)
    {
        PdhCloseQuery(m_query);
        m_query     = nullptr;
        m_engine    = nullptr;
        m_dedicated = nullptr;
        m_shared    = nullptr;
    }
    else
    {
        // Utilization Percentage is computed between two samples; the first collection
        // only establishes a baseline, so the first gather after this reports nothing.
        PdhCollectQueryData(m_query);
    }

    LOG_STARTUP(L"WinApiGpuProvider: initialized (%u adapter(s))%s",
                (uint32_t)m_adapters.size(),
                m_query ? L"" : L", GPU performance counters unavailable");
    return true;
}

bool WinApiGpuProvider::EnumerateAdapters()
{
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))) || !factory)
        return false;

    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            Adapter a;
            a.description    = desc.Description;
            a.vendorId       = desc.VendorId;
            a.dedicatedTotal = static_cast<double>(desc.DedicatedVideoMemory);
            a.sharedTotal    = static_cast<double>(desc.SharedSystemMemory);

            // PDH spells the LUID into its instance names as high then low, eight hex
            // digits each: "luid_0x00000000_0x0000EFE0_phys_0". The case of those digits
            // is not promised - current builds use upper, %x here gives lower - so every
            // comparison against this key goes through ContainsNoCase.
            wchar_t key[64];
            swprintf_s(key, L"luid_0x%08x_0x%08x",
                       static_cast<unsigned>(desc.AdapterLuid.HighPart),
                       static_cast<unsigned>(desc.AdapterLuid.LowPart));
            a.luidKey = key;

            LOG_STARTUP(L"WinApiGpuProvider: adapter %u %s dedicated=%.0f shared=%.0f",
                        (uint32_t)m_adapters.size(), a.luidKey.c_str(),
                        a.dedicatedTotal, a.sharedTotal);

            m_adapters.push_back(std::move(a));
        }

        adapter->Release();
        adapter = nullptr;
    }

    factory->Release();
    return !m_adapters.empty();
}

void WinApiGpuProvider::Shutdown()
{
    if (m_query)
    {
        PdhCloseQuery(m_query);
        m_query = nullptr;
    }
    m_adapters.clear();
}

uint32_t WinApiGpuProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_adapters.size());
}

int WinApiGpuProvider::FindAdapter(uint32_t pciVendorId, uint32_t ordinal) const
{
    uint32_t seen = 0;
    for (size_t i = 0; i < m_adapters.size(); ++i)
    {
        if (m_adapters[i].vendorId != pciVendorId)
            continue;
        if (seen++ == ordinal)
            return static_cast<int>(i);
    }
    return -1;
}

// PDH spells the LUID in its instance names with UPPERCASE hex on current Windows builds
// - "luid_0x00000000_0x0000EFE0_phys_0" - while swprintf's %x produces lowercase. A
// case-sensitive find therefore matched only on an adapter whose LUID happened to contain
// no hex letter, and silently matched nothing on one that did: every Windows-side metric
// read -1 on exactly the machines the Windows path exists for, an AMD GPU with no ADLX or
// an Intel one with no ControlLib. Neither spelling is promised by the API, so compare
// without case rather than betting on one.
static bool ContainsNoCase(const std::wstring& haystack, const std::wstring& needle)
{
    if (needle.empty())
        return true;

    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                       [](wchar_t a, wchar_t b) { return std::towlower(a) == std::towlower(b); })
           != haystack.end();
}

void WinApiGpuProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex >= m_adapters.size())
        return;

    const Adapter& ad = m_adapters[deviceIndex];

    // An integrated GPU has little or no dedicated memory and lives in system RAM, so
    // the shared pool is the number that means anything there.
    bool integrated = ad.dedicatedTotal <= 0.0;
    snap.Set(static_cast<uint32_t>(GpuMetric::VramTotal),
             integrated ? ad.sharedTotal : ad.dedicatedTotal);

    if (!m_query || PdhCollectQueryData(m_query) != ERROR_SUCCESS)
        return;

    std::vector<BYTE>            buf;
    PDH_FMT_COUNTERVALUE_ITEM_W* items = nullptr;
    DWORD                        count = 0;

    // Usage. Instance names are per process AND per engine
    // ("pid_7664_luid_..._phys_0_eng_1_engtype_3D"), so sum each engine type across
    // processes and report the busiest type. Task Manager shows the same thing: adding
    // every engine together would read far over 100% with video playing.
    if (m_engine && PdhReadCounterArray(m_engine, buf, items, count, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100))
    {
        std::map<std::wstring, double> byEngineType;
        for (DWORD i = 0; i < count; ++i)
        {
            std::wstring name = items[i].szName ? items[i].szName : L"";
            if (!ContainsNoCase(name, ad.luidKey))
                continue;

            size_t typePos = name.find(L"engtype_");
            std::wstring type = typePos == std::wstring::npos ? L"" : name.substr(typePos);
            byEngineType[type] += items[i].FmtValue.doubleValue;
        }

        if (!byEngineType.empty())
        {
            double busiest = 0.0;
            for (const auto& [type, used] : byEngineType)
                busiest = (std::max)(busiest, used);

            snap.Set(static_cast<uint32_t>(GpuMetric::Usage), (std::min)(busiest, 100.0));
        }
    }

    // VRAM in use, summed over every instance on this adapter. Unlike DXGI's
    // QueryVideoMemoryInfo, which only reports the calling process, this is the whole
    // machine's usage — which is what a desktop monitor is being asked for.
    auto sumForAdapter = [&](PDH_HCOUNTER counter, double& out) -> bool
    {
        if (!counter || !PdhReadCounterArray(counter, buf, items, count, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100))
            return false;

        double total = 0.0;
        bool   found = false;
        for (DWORD i = 0; i < count; ++i)
        {
            std::wstring name = items[i].szName ? items[i].szName : L"";
            if (!ContainsNoCase(name, ad.luidKey))
                continue;
            total += items[i].FmtValue.doubleValue;
            found = true;
        }

        out = total;
        return found;
    };

    double used = 0.0;
    if (sumForAdapter(integrated ? m_shared : m_dedicated, used))
        snap.Set(static_cast<uint32_t>(GpuMetric::VramUsed), used);

    // The shared pool is published for every adapter, not only integrated ones, because
    // it is the one memory figure no vendor SDK can see - and on an integrated GPU it is
    // where the real allocation lives, which is why a tool that sums both pools reads
    // higher than VramUsed does. On such a device VramUsed and VramShared are deliberately
    // the same number: there is one pool and both names point at it.
    double shared = 0.0;
    if (sumForAdapter(m_shared, shared))
        snap.Set(static_cast<uint32_t>(GpuMetric::VramShared), shared);
}

bool WinApiGpuProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_adapters.size() ||
        static_cast<GpuMetric>(metricId) != GpuMetric::Name)
        return false;

    out = m_adapters[deviceIndex].description;
    return !out.empty();
}
