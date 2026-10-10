#include <windows.h>

#include "Plugin/Plugin.h"
#include "Core/HardwareCore.h"
#include "Types/MetricParser.h"
#include "Utils/Debug.h"

#include "RainmeterAPI.h"

#include <cstdlib>
#include <string>

// Debug=2 diagnostic dump. Overwrites %TEMP%\NativeHardwareMonitor\<Category>_dev<N>.txt
// with every metric for this measure's category+device — the file to send back for
// hardware we can't test locally (e.g. Intel GPUs). Throttled per measure.
static void MaybeDump(MeasureContext* ctx)
{
    if (ctx->debugLevel < 2 || !ctx->valid)
        return;

    uint64_t now = GetTickCount64();
    if (ctx->lastDumpMs != 0 && now - ctx->lastDumpMs < 2000) // ponytail: 2s is plenty for a debug snapshot
        return;
    ctx->lastDumpMs = now;

    wchar_t dir[MAX_PATH];
    if (GetTempPathW(MAX_PATH, dir) == 0)
        return;
    std::wstring folder = std::wstring(dir) + L"NativeHardwareMonitor";
    CreateDirectoryW(folder.c_str(), nullptr); // no-op if it already exists

    std::wstring path = folder + L"\\" + CategoryName(ctx->category) +
                        L"_dev" + std::to_wstring(ctx->deviceIndex) + L".txt";
    HardwareCore::Instance().DumpDevice(ctx->category, ctx->deviceIndex, path);
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

    if (ctx->debugLevel > g_DebugLevel)
        g_DebugLevel = ctx->debugLevel;

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

    if (ctx->valid)
    {
        GetCore().UnregisterMeasure(ctx->handle);
        ctx->valid = false;
    }

    // Host=/PingInterval= may have changed and Ping handles were registered
    // against the old values. Dropping them is cheaper than diffing; they
    // re-register on the next section-variable read.
    ReleaseSectionHandles(ctx);

    ReadContext(ctx, rm);

    if (ctx->debugLevel > g_DebugLevel)
        g_DebugLevel = ctx->debugLevel;

    if (ctx->metricId == static_cast<uint32_t>(-1))
    {
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
// exported function taking (metric, [device]); the metric name goes through the
// same ParseMetric as Metric=, so every alias works here too.
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
    if (metric != METRIC_DEVICE_COUNT && metric >= MetricCount(category))
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

// Section variables are substituted into text as-is: Text= has no formula to
// round with, and NumOfDecimals only ever applies to a measure's value. So the
// printing happens here. Three decimals with trailing zeros trimmed is past any
// sensor's real precision and leaves whole numbers (and byte counts) whole; an
// explicit third argument overrides it - [NHM:CPU(usage,,0)].
static void FormatValue(double value, int decimals, std::wstring& out)
{
    wchar_t buf[64];

    if (decimals >= 0)
    {
        swprintf_s(buf, L"%.*f", decimals, value);
        out = buf;
        return;
    }

    swprintf_s(buf, L"%.3f", value);
    out = buf;

    size_t last = out.find_last_not_of(L'0');
    if (last != std::wstring::npos && out[last] == L'.')
        --last; // nothing left after the point - drop it too

    out.erase(last + 1);
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

    // Optional third argument: decimals. An omitted middle argument is an empty
    // string, so [NHM:CPU(usage,,0)] still means device 0.
    int decimals = -1;
    if (argc > 2 && argv[2][0] != L'\0')
    {
        decimals = _wtoi(argv[2]);
        if (decimals < 0)  decimals = 0;
        if (decimals > 15) decimals = 15;
    }

    // Same order Rainmeter uses on a measure: the value first (it initializes the
    // module on the first read), then the string form if the metric has one.
    double value = GetCore().GetValue(handle);

    std::wstring text;
    if (GetCore().GetString(handle, text))
        ctx->svBuffer = text; // string metric: Name, DriverVersion, ThrottleReasons, ...
    else
        FormatValue(value, decimals, ctx->svBuffer);

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
