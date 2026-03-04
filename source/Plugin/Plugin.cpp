#include <windows.h>

#include "Plugin/Plugin.h"
#include "Core/HardwareCore.h"

#include "Types/MetricParser.h"

#include "RainmeterAPI.h"

// ------------------------------------------------------------
// Core access
// ------------------------------------------------------------

static HardwareCore& GetCore()
{
    return HardwareCore::Instance();
}

// ------------------------------------------------------------
// Initialize
// ------------------------------------------------------------

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
    auto* ctx = new MeasureContext();

    LPCWSTR categoryRaw = RmReadString(rm, L"Category", L"", FALSE);
    std::wstring categoryStr = categoryRaw ? categoryRaw : L"";

    LPCWSTR metricRaw = RmReadString(rm, L"Metric", L"", FALSE);
    std::wstring metricStr = metricRaw ? metricRaw : L"";

    ctx->category = ParseCategory(categoryStr);
    ctx->metricId = ParseMetric(ctx->category, metricStr);

    if (ctx->metricId == static_cast<uint32_t>(-1))
    {
        RmLog(rm, LOG_ERROR, L"[NHM] Metric not supported for this category.");
        *data = ctx;
        return;
    }

    ctx->deviceIndex = static_cast<uint32_t>(
        RmReadInt(rm, L"Device", 0)
        );

    ctx->handle = GetCore().RegisterMeasure(
        ctx->category,
        ctx->metricId,
        ctx->deviceIndex
    );

    ctx->valid = true;

    *data = ctx;
}

// ------------------------------------------------------------
// Reload
// ------------------------------------------------------------

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

    LPCWSTR categoryRaw = RmReadString(rm, L"Category", L"", FALSE);
    std::wstring categoryStr = categoryRaw ? categoryRaw : L"";

    LPCWSTR metricRaw = RmReadString(rm, L"Metric", L"", FALSE);
    std::wstring metricStr = metricRaw ? metricRaw : L"";

    ctx->category = ParseCategory(categoryStr);
    ctx->metricId = ParseMetric(ctx->category, metricStr);

    if (ctx->metricId == static_cast<uint32_t>(-1))
    {
        RmLog(rm, LOG_ERROR, L"[NHM] Metric not supported for this category.");
        return;
    }

    ctx->deviceIndex = static_cast<uint32_t>(
        RmReadInt(rm, L"Device", 0)
        );

    ctx->handle = GetCore().RegisterMeasure(
        ctx->category,
        ctx->metricId,
        ctx->deviceIndex
    );

    ctx->valid = true;
}
// ------------------------------------------------------------
// Update
// ------------------------------------------------------------

PLUGIN_EXPORT double Update(void* data)
{
    auto* ctx = static_cast<MeasureContext*>(data);

    if (!ctx || !ctx->valid)
        return 0.0;

    return GetCore().GetValue(ctx->handle);
}

// ------------------------------------------------------------
// Finalize
// ------------------------------------------------------------

PLUGIN_EXPORT void Finalize(void* data)
{
    auto* ctx = static_cast<MeasureContext*>(data);

    if (!ctx)
        return;

    if (ctx->valid)
    {
        GetCore().UnregisterMeasure(ctx->handle);
    }

    delete ctx;
}