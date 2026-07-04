#include <windows.h>

#include "Plugin/Plugin.h"
#include "Core/HardwareCore.h"
#include "Types/MetricParser.h"
#include "Utils/Debug.h"

#include "RainmeterAPI.h"

static HardwareCore& GetCore()
{
    return HardwareCore::Instance();
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

    ReadContext(ctx, rm);

    if (ctx->debugLevel > g_DebugLevel)
        g_DebugLevel = ctx->debugLevel;

    if (ctx->metricId == static_cast<uint32_t>(-1))
    {
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
    if (!ctx || !ctx->valid)
        return -2.0;

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

PLUGIN_EXPORT void Finalize(void* data)
{
    auto* ctx = static_cast<MeasureContext*>(data);
    if (!ctx)
        return;

    if (ctx->valid)
        GetCore().UnregisterMeasure(ctx->handle);

    delete ctx;
}
