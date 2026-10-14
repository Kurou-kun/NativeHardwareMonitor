// <windows.h> drags in the original winsock.h unless this is defined, and this file now
// constructs WinApiPingProvider, whose header needs <winsock2.h> - the two redefine each
// other's structs. The per-category Resolver classes used to hide the problem by including
// their provider from the .cpp instead of the header. Nothing here needs the parts that
// WIN32_LEAN_AND_MEAN omits, and Core/HardwareCore.h -> PollerThread.h is the first include
// to reach <windows.h>, so the define has to sit above it.
#define WIN32_LEAN_AND_MEAN

#include "Core/HardwareCore.h"
#include "Types/MetricParser.h"
#include "Utils/Debug.h"

#include "Core/Module.h"

#include "Providers/WinApi/WinApiPingProvider.h"

#include "Modules/GPU/GpuResolver.h"
#include "Providers/WinApi/WinApiCpuProvider.h"
#include "Providers/WinApi/WinApiMemoryProvider.h"
#include "Providers/WinApi/WinApiNetworkProvider.h"
#include "Providers/WinApi/WinApiStorageProvider.h"
#include "Providers/WinApi/WinApiBatteryProvider.h"
#include "Providers/WinApi/WinApiSystemProvider.h"
#include "Providers/WinApi/WinApiMotherboardProvider.h"
#include "Providers/WinApi/WinApiDisplayProvider.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>
#include <windows.h>

// Default poll cadence, matching a typical skin's Update=1000. UpdateOverride= only
// tightens it (see UpdatePoller).
static constexpr uint32_t DefaultIntervalMs = 1000;

HardwareCore& HardwareCore::Instance()
{
    // Leaked on purpose, for the same reason as g_ADLX in AdlxProvider.cpp: a
    // static instance is destroyed from DllMain(DLL_PROCESS_DETACH) under the
    // loader lock, where this destructor would join every poller thread (a thread
    // cannot run its DLL_THREAD_DETACH there) and call vendor teardown -
    // nvmlShutdown, NVAPI unload, ADL2 destroy, FreeLibrary. That deadlocks, or
    // frees a module out from under a gather still in flight, which is the
    // nvml.dll c0000005 that WER logged ten times on 2026-10-10.
    static HardwareCore* instance = new HardwareCore();
    return *instance;
}

HardwareCore::HardwareCore()
{
    // Pin the module. Rainmeter frees the plugin DLL as soon as the last
    // NativeHardwareMonitor measure disappears - deactivating the only skin that
    // uses it, for instance - while the poller threads started below are still
    // running. Those threads then execute unmapped code: Rainmeter died with
    // c0000005 at NativeHardwareMonitor.dll_unloaded+0x906c on 2.1.4, which is
    // what Yincognito hit unloading any of the test skins. Pinning makes that
    // unload a no-op so our code stays mapped; polling is stopped separately in
    // UnregisterMeasure, so a pinned-but-idle module costs nothing. Nothing is
    // lost either way: a DLL a loaded skin has mapped was never replaceable.
    HMODULE self = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        reinterpret_cast<LPCWSTR>(&HardwareCore::Instance),
        &self);

    // One Module class serves every category; what differs is the provider behind it and
    // the noun its startup log counts in. The per-category Module and Resolver classes this
    // replaced were identical boilerplate - see Core/Module.h.
    auto add = [this](Category category, std::unique_ptr<IProvider> provider,
                      const wchar_t* name, const wchar_t* noun = nullptr)
    {
        auto state = std::make_unique<ModuleState>();
        state->module = std::make_unique<Module>(category, std::move(provider), name, noun);
        m_modules[category] = std::move(state);
    };

    add(Category::GPU,     std::make_unique<GpuResolver>(),               L"GpuResolver", L"device(s)");
    add(Category::CPU,     std::make_unique<WinApiCpuProvider>(),         L"WinApiCpuProvider", L"device(s), incl. total");
    add(Category::Memory,  std::make_unique<WinApiMemoryProvider>(),      L"WinApiMemoryProvider");
    add(Category::Network, std::make_unique<WinApiNetworkProvider>(),     L"WinApiNetworkProvider", L"adapter(s)");
    add(Category::Storage, std::make_unique<WinApiStorageProvider>(),     L"WinApiStorageProvider", L"drive(s)");
    add(Category::Ping,    std::make_unique<WinApiPingProvider>(),        L"WinApiPingProvider");
    add(Category::Battery, std::make_unique<WinApiBatteryProvider>(),     L"WinApiBatteryProvider");
    add(Category::System,  std::make_unique<WinApiSystemProvider>(),      L"WinApiSystemProvider");
    add(Category::Motherboard, std::make_unique<WinApiMotherboardProvider>(), L"WinApiMotherboardProvider");
    add(Category::Display, std::make_unique<WinApiDisplayProvider>(),     L"WinApiDisplayProvider", L"display(s)");
}

uint32_t HardwareCore::RegisterMeasure(
    Category            category,
    uint32_t             metricId,
    uint32_t             deviceIndex,
    uint32_t             updateOverrideMs,
    const std::wstring& targetHost,
    uint32_t             pingIntervalMs)
{
    uint32_t handle = m_nextHandle++;

    if (category == Category::Ping)
    {
        // Ping devices are resolved from a host string, not a skin-supplied
        // Device= index — and unlike other categories, it must be initialized
        // here (at registration time) rather than lazily on first read, since
        // the background ping polling has to start the moment the skin loads.
        ModuleState* state = GetModuleState(category);
        if (state && state->module)
        {
            // ResolveTarget grows the module's snapshot vector — must not race
            // a PollerThread that's mid-GatherAll on this module.
            std::lock_guard<std::timed_mutex> lock(state->mutex);

            if (!state->initialized)
                state->initialized = state->module->Initialize();

            if (state->initialized)
                deviceIndex = state->module->ResolveTarget(targetHost, pingIntervalMs);
        }
    }

    m_measures.emplace(handle, MeasureEntry{ category, metricId, deviceIndex, updateOverrideMs });

    RebuildWantedDevices(category);

    if (ModuleState* state = GetModuleState(category))
        EnsurePoller(*state);   // nothing may gather on Rainmeter's thread

    if (updateOverrideMs > 0)
    {
        ModuleState* state = GetModuleState(category);
        if (state)
        {
            state->overrides[handle] = updateOverrideMs;
            UpdatePoller(*state);
        }
    }

    return handle;
}

void HardwareCore::UnregisterMeasure(uint32_t handle)
{
    auto it = m_measures.find(handle);
    if (it == m_measures.end())
        return;

    const MeasureEntry& entry = it->second;

    if (entry.updateOverrideMs > 0)
    {
        ModuleState* state = GetModuleState(entry.category);
        if (state)
        {
            state->overrides.erase(handle);
            UpdatePoller(*state); // may stop poller — must NOT hold mutex here
        }
    }

    Category erased = entry.category;
    m_measures.erase(it);
    RebuildWantedDevices(erased);

    // No measures left: stop polling hardware until one comes back. Rainmeter
    // unloads the plugin at this point (see the pin in the constructor), and a
    // poller left running would keep calling vendor drivers on behalf of a skin
    // that is gone. Stop() signals and returns - joining here would stall
    // Rainmeter's thread for the length of a gather.
    if (m_measures.empty())
    {
        for (auto& [category, state] : m_modules)
        {
            state->poller.reset();   // ~PollerThread signals, does not join
            state->currentOverrideMs = 0;
            state->overrides.clear();
        }
    }
}

// Which devices anyone is reading, per category. Rebuilt on registration rather than per
// tick: measures come and go when a skin loads, not between gathers.
void HardwareCore::RebuildWantedDevices(Category category)
{
    ModuleState* state = GetModuleState(category);
    if (!state)
        return;

    std::vector<uint32_t> wanted;
    for (const auto& [handle, entry] : m_measures)
    {
        if (entry.category != category)
            continue;
        if (std::find(wanted.begin(), wanted.end(), entry.deviceIndex) == wanted.end())
            wanted.push_back(entry.deviceIndex);
    }

    state->wantedDevices = std::move(wanted);
}

double HardwareCore::GetValue(uint32_t handle)
{
    auto it = m_measures.find(handle);
    if (it == m_measures.end())
        return -2.0;

    MeasureEntry& entry = it->second;
    ModuleState* state = GetModuleState(entry.category);
    if (!state || !state->module)
        return -2.0;

    // Rainmeter's own thread calls this. It must never touch a vendor driver and
    // must never wait on one: Initialize() and GatherAll() belong to the poller
    // (see EnsurePoller), and a gather in flight holds the lock, so a reader that
    // cannot take it immediately returns the value it last reported rather than
    // blocking. A stalled NVML/ADL call used to freeze Rainmeter outright here.
    // Device count needs no snapshot and must never come back negative - a skin
    // clamps a device index against it, so -1 here turns Clamp(x, 0, count-1) into
    // Clamp(x, 0, -2) and the skin starts reading a device that cannot exist. The
    // poller publishes it; 0 means "not known yet", which clamps harmlessly.
    if (entry.metricId == METRIC_DEVICE_COUNT)
        return static_cast<double>(state->deviceCount.load(std::memory_order_relaxed));

    if (entry.metricId == METRIC_PROVIDER_STATUS)
        return static_cast<double>(state->status.load(std::memory_order_relaxed));

    std::unique_lock<std::timed_mutex> lock(state->mutex, std::defer_lock);
    if (!lock.try_lock_for(std::chrono::milliseconds(2)))
        return entry.lastValue;

    if (!state->initialized)
        return entry.lastValue;   // poller has not finished the first gather yet

    entry.lastValue = state->module->GetValue(entry.metricId, entry.deviceIndex);
    return entry.lastValue;
}

bool HardwareCore::GetString(uint32_t handle, std::wstring& out)
{
    auto it = m_measures.find(handle);
    if (it == m_measures.end())
        return false;

    const MeasureEntry& entry = it->second;
    ModuleState* state = GetModuleState(entry.category);
    if (!state || !state->module)
        return false;

    std::unique_lock<std::timed_mutex> lock(state->mutex, std::defer_lock);
    if (!lock.try_lock_for(std::chrono::milliseconds(2)))
        return false;   // gather in flight: Rainmeter keeps the previous string

    if (!state->initialized)
        return false;

    if (entry.metricId == METRIC_DEVICE_COUNT || entry.metricId == METRIC_PROVIDER_STATUS)
        return false; // numeric only — a module must never see a reserved id

    return state->module->GetString(entry.metricId, entry.deviceIndex, out);
}

uint32_t HardwareCore::DeviceCount(Category category)
{
    ModuleState* state = GetModuleState(category);
    if (!state || !state->module)
        return 0;

    return state->deviceCount.load(std::memory_order_relaxed);
}

void HardwareCore::DumpDevice(Category category, uint32_t deviceIndex, const std::wstring& path)
{
    ModuleState* state = GetModuleState(category);
    if (!state || !state->module)
        return;

    std::wstring body;
    {
        // Diagnostic, and on Rainmeter's thread: skip this snapshot rather than
        // wait on a gather. It is throttled anyway, so the next tick picks it up.
        std::unique_lock<std::timed_mutex> lock(state->mutex, std::defer_lock);
        if (!lock.try_lock_for(std::chrono::milliseconds(50)))
            return;

        if (!state->initialized)
            return;   // the poller initialises; nothing to dump yet

        // A dump exists to show devices no measure reads - that is how the AMD iGPU
        // enumeration bug was found - so from the first dump this category goes back to
        // gathering everything. Deliberately not gathered here: doing that on
        // Rainmeter's thread under the module mutex is what wedged the app before 2.1.3.
        state->dumpAll = true;

        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t hdr[256];
        swprintf_s(hdr,
            L"NativeHardwareMonitor (built %S %S)\r\n%s / device %u of %u  -  %04u-%02u-%02u %02u:%02u:%02u\r\n\r\n",
            __DATE__, __TIME__, CategoryName(category).c_str(), deviceIndex,
            state->module->GetDeviceCount(),
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        body = hdr;

        // The provider story, carried by the dump itself. These lines are written once,
        // when each provider first initialises, so by the time someone thinks to attach a
        // log they have scrolled off - a reporter concluded PMLog was unavailable on his
        // driver for exactly that reason when it was working fine.
        body += L"--- providers ---\r\n";
        for (const std::wstring& line : StartupLog())
            body += L"  " + line + L"\r\n";
        body += L"\r\n";

        uint32_t count = MetricCount(category);
        for (uint32_t id = 0; id < count; ++id)
        {
            std::wstring name = MetricName(category, id);
            if (name.size() < 20) name.append(20 - name.size(), L' ');

            std::wstring s;
            if (state->module->GetString(id, deviceIndex, s))
            {
                body += name + L"= \"" + s + L"\"\r\n";
            }
            else
            {
                double v = state->module->GetValue(id, deviceIndex);
                wchar_t val[64];
                if (v == -1.0)      swprintf_s(val, L"%g  (unsupported)", v);
                else if (v == -2.0) swprintf_s(val, L"%g  (error)", v);
                else                swprintf_s(val, L"%g", v);
                body += name + L"= " + val;

                // Which API produced it. GPU is why this exists: NVML and NVAPI can both
                // answer for a clock and disagree, and a dump that does not say which one
                // spoke turns a bug report into guesswork.
                if (const wchar_t* src = state->module->GetSource(id, deviceIndex))
                    body += std::wstring(L"  [") + src + L"]";

                body += L"\r\n";
            }
        }
    }

    int n = WideCharToMultiByte(CP_UTF8, 0, body.c_str(), static_cast<int>(body.size()),
                                nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return;
    std::string utf8(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, body.c_str(), static_cast<int>(body.size()),
                        utf8.data(), n, nullptr, nullptr);

    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (f)
        f.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
}

HardwareCore::ModuleState* HardwareCore::GetModuleState(Category category)
{
    auto it = m_modules.find(category);
    if (it == m_modules.end())
        return nullptr;

    return it->second.get();
}

void HardwareCore::UpdatePoller(ModuleState& state)
{
    if (state.overrides.empty())
    {
        // Do not Stop() here: that joins the poller thread from Rainmeter's own
        // thread, which stalls it for as long as a gather takes (up to the 5 s
        // timeout) every time a skin with an UpdateOverride refreshes.
        if (state.currentOverrideMs != 0)
        {
            state.currentOverrideMs = 0;
            StartPoller(state, DefaultIntervalMs);
        }
        return;
    }

    uint32_t minMs = UINT32_MAX;
    for (auto& [h, ms] : state.overrides)
        minMs = std::min(minMs, ms);

    if (minMs == state.currentOverrideMs)
        return;

    state.currentOverrideMs = minMs;

    if (state.poller)
        state.poller->Stop();
    else
        state.poller = std::make_unique<PollerThread>();

    StartPoller(state, minMs);
}

// Every category polls on its own thread, whether or not a skin asked for an
// UpdateOverride, because the alternative is gathering on Rainmeter's thread.
// Default cadence matches a typical skin's Update=1000; UpdateOverride= tightens it.
void HardwareCore::EnsurePoller(ModuleState& state)
{
    if (state.poller)
        return;

    state.currentOverrideMs = 0;
    StartPoller(state, DefaultIntervalMs);
}

void HardwareCore::StartPoller(ModuleState& state, uint32_t intervalMs)
{
    if (!state.poller)
        state.poller = std::make_unique<PollerThread>();

    // The ModuleState outlives its poller (the mutex is declared before it and so is
    // destroyed after), so capturing it alone is enough - module, mutex, flag and count
    // all hang off it.
    ModuleState* st = &state;

    state.poller->Start(intervalMs, [st]()
    {
        IModule* module = st->module.get();

        std::lock_guard<std::timed_mutex> lock(st->mutex);
        if (!st->initialized)
        {
            st->initialized = module->Initialize();

            // A provider that cannot initialise will not succeed on a later tick either -
            // a missing SDK stays missing - so this is the one place that can say so.
            if (!st->initialized)
                st->status.store(-1, std::memory_order_relaxed);
        }
        if (st->initialized)
        {
            if (st->dumpAll)
                module->GatherAll();
            else
                module->GatherDevices(st->wantedDevices);

            st->deviceCount.store(module->GetDeviceCount(), std::memory_order_relaxed);
            st->status.store(1, std::memory_order_relaxed);
        }
    });
}

