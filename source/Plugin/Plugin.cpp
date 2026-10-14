#include <windows.h>

#include "Plugin/Plugin.h"
#include "Core/HardwareCore.h"
#include "Types/MetricParser.h"
#include "Utils/Debug.h"
#include "Utils/ValueFormat.h"

#include "RainmeterAPI.h"

#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>

// Debug=2 diagnostic dump. Overwrites %TEMP%\NativeHardwareMonitor\<Category>_dev<N>.txt
// with every metric for every device in the category — the files to send back for
// hardware we can't test locally (e.g. Intel GPUs).
//
// Throttled PER CATEGORY, not per measure: a dump calls GatherAll() under the module
// mutex on Rainmeter's own thread, and the test skins put Debug=2 on every measure
// (gpu.ini has 26). Per-measure throttling let that become ~13 full hardware gathers a
// second on the UI thread, which wedges Rainmeter. One dump per category per interval is
// all a diagnostic needs - every measure reads the same snapshot anyway.
static void MaybeDump(MeasureContext* ctx)
{
    if (ctx->debugLevel < 2 || !ctx->valid)
        return;

    {
        // ponytail: one flat mutex over a 10-entry map, contended once per tick at worst.
        static std::mutex                             dumpMutex;
        static std::unordered_map<Category, uint64_t> lastDumpMs;

        uint64_t now = GetTickCount64();
        std::lock_guard<std::mutex> lock(dumpMutex);

        auto it = lastDumpMs.find(ctx->category);
        if (it != lastDumpMs.end() && now - it->second < 2000) // 2s is plenty for a snapshot
            return;
        lastDumpMs[ctx->category] = now;
    }

    wchar_t dir[MAX_PATH];
    if (GetTempPathW(MAX_PATH, dir) == 0)
        return;
    std::wstring folder = std::wstring(dir) + L"NativeHardwareMonitor";
    CreateDirectoryW(folder.c_str(), nullptr); // no-op if it already exists

    // Dump every device in the category, not just this measure's own. Diagnosing a
    // device that is missing needs a dump of a device no measure can validly read -
    // that is how the AMD iGPU enumeration bug stayed hidden: the reporter's measure
    // could only ever write GPU_dev0.txt.
    uint32_t count = HardwareCore::Instance().DeviceCount(ctx->category);
    uint32_t first = 0, last = count;
    if (count == 0) // nothing enumerated: still dump the requested index, header shows "0 of 0"
    {
        first = ctx->deviceIndex;
        last  = ctx->deviceIndex + 1;
    }

    for (uint32_t dev = first; dev < last; ++dev)
    {
        std::wstring path = folder + L"\\" + CategoryName(ctx->category) +
                            L"_dev" + std::to_wstring(dev) + L".txt";
        HardwareCore::Instance().DumpDevice(ctx->category, dev, path);
    }
}

static HardwareCore& GetCore()
{
    return HardwareCore::Instance();
}

// Hands every lazily registered section-variable handle back to the core.
static void ReleaseSectionHandles(MeasureContext* ctx)
{
    for (const SectionHandle& sh : ctx->svHandles)
    {
        if (sh.handle != SV_BAD_METRIC)
            GetCore().UnregisterMeasure(sh.handle);
    }

    ctx->svHandles.clear();
}

static void ReadContext(MeasureContext* ctx, void* rm)
{
    // Each RmReadString returns a pointer into Rainmeter's internal buffer —
    // assign to wstring immediately before the next call overwrites it.
    ctx->categoryStr = RmReadString(rm, L"Category", L"", FALSE);
    ctx->metricStr   = RmReadString(rm, L"Metric",   L"", FALSE);
    ctx->host        = RmReadString(rm, L"Host",     L"", FALSE);

    ctx->category         = ParseCategory(ctx->categoryStr);
    ctx->metricId         = ParseMetric(ctx->category, ctx->metricStr);
    ctx->hub              = ctx->categoryStr.empty();
    ctx->deviceIndex      = static_cast<uint32_t>(RmReadInt(rm, L"Device", 0));
    ctx->debugLevel       = RmReadInt(rm, L"Debug", 0);
    ctx->updateOverrideMs = static_cast<uint32_t>(RmReadInt(rm, L"UpdateOverride", 0));
    ctx->pingIntervalMs   = static_cast<uint32_t>(RmReadInt(rm, L"PingInterval", 1000));
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
    g_Rainmeter = rm;

    auto* ctx = new MeasureContext();

    ReadContext(ctx, rm);

    if (ctx->metricId == static_cast<uint32_t>(-1))
    {
        // No Category= at all is deliberate: the measure exists only to host
        // section variables ([MeasureName:GPU(temp)]), so it stays quiet. A
        // *wrong* Category= is a typo and still says so.
        if (!ctx->hub)
            LOG_MEASURE(rm, ctx->debugLevel, L"Category='%s' Metric='%s' dev%u — unknown category, skipped",
                ctx->categoryStr.c_str(), ctx->metricStr.c_str(), ctx->deviceIndex);
        *data = ctx;
        return;
    }

    ctx->handle = GetCore().RegisterMeasure(
        ctx->category,
        ctx->metricId,
        ctx->deviceIndex,
        ctx->updateOverrideMs,
        ctx->host,
        ctx->pingIntervalMs
    );

    ctx->valid = true;

    LOG_MEASURE(rm, ctx->debugLevel, L"[%s/%s/dev%u] Registered (handle=%u)",
        ctx->categoryStr.c_str(), ctx->metricStr.c_str(),
        ctx->deviceIndex, ctx->handle);

    *data = ctx;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
    auto* ctx = static_cast<MeasureContext*>(data);
    if (!ctx)
        return;

    // Register the new handle BEFORE dropping the old one, never the other way round.
    // Rainmeter calls Reload on every single update when a measure sets
    // DynamicVariables=1 (measured: once per second per dynamic measure), and
    // UnregisterMeasure stops every poller the moment the measure map empties. In a skin
    // whose only NHM measure is a dynamic one, unregistering first empties that map for
    // the instant between the two calls, so the category's poller is torn down and
    // recreated once a second and only lands a gather by winning a race with the next
    // tick. Keeping one handle registered throughout removes the window entirely.
    uint32_t oldHandle = ctx->handle;
    bool     hadOld    = ctx->valid;
    ctx->valid = false;

    // Host=/PingInterval= may have changed and Ping handles were registered
    // against the old values. Dropping them is cheaper than diffing; they
    // re-register on the next section-variable read.
    ReleaseSectionHandles(ctx);

    ReadContext(ctx, rm);

    if (ctx->metricId == static_cast<uint32_t>(-1))
    {
        if (hadOld)
            GetCore().UnregisterMeasure(oldHandle);

        if (!ctx->hub)
            LOG_MEASURE(rm, ctx->debugLevel, L"[%s] Unknown category — skipped",
                ctx->categoryStr.c_str());
        return;
    }

    ctx->handle = GetCore().RegisterMeasure(
        ctx->category,
        ctx->metricId,
        ctx->deviceIndex,
        ctx->updateOverrideMs,
        ctx->host,
        ctx->pingIntervalMs
    );

    ctx->valid = true;

    if (hadOld)
        GetCore().UnregisterMeasure(oldHandle);

    LOG_MEASURE(rm, ctx->debugLevel, L"[%s/%s/dev%u] Reloaded (handle=%u)",
        ctx->categoryStr.c_str(), ctx->metricStr.c_str(),
        ctx->deviceIndex, ctx->handle);
}

PLUGIN_EXPORT double Update(void* data)
{
    auto* ctx = static_cast<MeasureContext*>(data);
    if (!ctx)
        return -2.0;

    if (!ctx->valid)
        return ctx->hub ? 0.0 : -2.0; // a hub measure reads 0, not an error

    MaybeDump(ctx);

    return GetCore().GetValue(ctx->handle);
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
    auto* ctx = static_cast<MeasureContext*>(data);
    if (!ctx || !ctx->valid)
        return nullptr;

    static thread_local std::wstring result;

    if (!GetCore().GetString(ctx->handle, result))
        return nullptr;

    return result.c_str();
}

// ---------------------------------------------------------------------------
// Section variables — [MeasureName:GPU(temp)], [MeasureName:CPU(usage,0)]
//
// Rainmeter resolves these against one measure, so a single hub measure can
// serve a whole skin instead of one measure per metric. Each category gets one
// exported function taking (metric, [device], [decimals], [scale]); the metric
// name goes through the same ParseMetric as Metric=, so every alias works here
// too, and the two formatting arguments exist because Rainmeter's own cannot
// reach a plugin section variable at all - see Utils/ValueFormat.h.
//
// Reading many metrics this way costs one handle each but still only one poller
// per category — which is the point: one measure, no per-metric measures.
// ---------------------------------------------------------------------------

static uint32_t SectionHandleFor(MeasureContext* ctx, Category category, int argc, const WCHAR* argv[])
{
    const std::wstring name = argc > 0 ? argv[0] : L"";

    uint32_t metric = ParseMetric(category, name);
    uint32_t device = argc > 1 ? static_cast<uint32_t>(_wtoi(argv[1])) : 0;

    // ponytail: linear scan — a hub measure serves a handful of metrics, so the
    // vector beats hashing a triple. Swap in a map if anyone reads hundreds.
    for (const SectionHandle& sh : ctx->svHandles)
    {
        if (sh.category == category && sh.metric == metric && sh.device == device)
            return sh.handle;
    }

    // An unrecognised name lands on the category's Unknown sentinel, which would
    // otherwise read -1 forever without saying why. Say it once, then remember.
    if (metric != METRIC_DEVICE_COUNT && metric != METRIC_PROVIDER_STATUS &&
        metric >= MetricCount(category))
    {
        LOG_ERROR(L"[%s] Unknown metric '%s' in section variable",
            CategoryName(category).c_str(), name.c_str());
        ctx->svHandles.push_back({ category, metric, device, SV_BAD_METRIC });
        return SV_BAD_METRIC;
    }

    uint32_t handle = GetCore().RegisterMeasure(
        category,
        metric,
        device,
        0, // no UpdateOverride: section variables are pulled, not on a clock of their own
        ctx->host,
        ctx->pingIntervalMs
    );

    ctx->svHandles.push_back({ category, metric, device, handle });

    LOG_MEASURE(g_Rainmeter, ctx->debugLevel, L"[%s/%s/dev%u] Section variable registered (handle=%u)",
        CategoryName(category).c_str(), name.c_str(), device, handle);

    return handle;
}

static LPCWSTR SectionValue(void* data, Category category, int argc, const WCHAR* argv[])
{
    auto* ctx = static_cast<MeasureContext*>(data);
    if (!ctx)
        return L"-2";

    uint32_t handle = SectionHandleFor(ctx, category, argc, argv);
    if (handle == SV_BAD_METRIC)
    {
        ctx->svBuffer = L"-1";
        return ctx->svBuffer.c_str();
    }

    // Optional third and fourth arguments: decimals and scale - [NHM:CPU(usage,,0)],
    // [NHM:Storage(freespace,,1,auto)]. ValueFormat.h has the why and the full syntax.
    const WCHAR* decimalsArg = argc > 2 ? argv[2] : nullptr;
    const WCHAR* scaleArg    = argc > 3 ? argv[3] : nullptr;

    // Same order Rainmeter uses on a measure: the value first (it initializes the
    // module on the first read), then the string form if the metric has one.
    double value = GetCore().GetValue(handle);

    std::wstring text;
    if (GetCore().GetString(handle, text))
        ctx->svBuffer = text; // string metric: Name, DriverVersion, ThrottleReasons, ...
    else
        FormatSectionValue(value, decimalsArg, scaleArg, ctx->svBuffer);

    return ctx->svBuffer.c_str();
}

PLUGIN_EXPORT LPCWSTR GPU(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::GPU, argc, argv);
}

PLUGIN_EXPORT LPCWSTR CPU(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::CPU, argc, argv);
}

PLUGIN_EXPORT LPCWSTR Memory(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::Memory, argc, argv);
}

PLUGIN_EXPORT LPCWSTR Network(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::Network, argc, argv);
}

PLUGIN_EXPORT LPCWSTR Storage(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::Storage, argc, argv);
}

PLUGIN_EXPORT LPCWSTR Ping(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::Ping, argc, argv);
}

PLUGIN_EXPORT LPCWSTR Battery(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::Battery, argc, argv);
}

PLUGIN_EXPORT LPCWSTR System(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::System, argc, argv);
}

PLUGIN_EXPORT LPCWSTR Motherboard(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::Motherboard, argc, argv);
}

PLUGIN_EXPORT LPCWSTR Display(void* data, const int argc, const WCHAR* argv[])
{
    return SectionValue(data, Category::Display, argc, argv);
}

PLUGIN_EXPORT void Finalize(void* data)
{
    auto* ctx = static_cast<MeasureContext*>(data);
    if (!ctx)
        return;

    if (ctx->valid)
        GetCore().UnregisterMeasure(ctx->handle);

    ReleaseSectionHandles(ctx);

    delete ctx;
}
