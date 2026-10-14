#pragma once

#include <cstdint>
#include <unordered_map>

struct MetricValue
{
    double value     = 0.0;
    bool   supported = false;

    // Which provider produced this reading. A literal owned by the provider chain, never
    // freed, so a bare pointer is right. It exists because a dump that says
    // "CoreClock = 1.1e+09" leaves the one question a bug report turns on unanswered:
    // WHICH API said so. Two providers disagreeing about a gated laptop dGPU's clock is
    // not a hypothetical - it is forum post p=246235.
    const wchar_t* source = nullptr;
};

struct Snapshot
{
    std::unordered_map<uint32_t, MetricValue> metrics;

    // Stamped onto every Set() that follows. The caller that drives a provider sets it,
    // so providers stay unaware of their own name and nothing in them had to change.
    const wchar_t* currentSource = nullptr;

    void Set(uint32_t metricId, double value)
    {
        metrics[metricId] = { value, true, currentSource };
    }

    double Get(uint32_t metricId) const
    {
        auto it = metrics.find(metricId);
        if (it == metrics.end() || !it->second.supported)
            return -1.0;

        return it->second.value;
    }

    const wchar_t* SourceOf(uint32_t metricId) const
    {
        auto it = metrics.find(metricId);
        return it != metrics.end() && it->second.supported ? it->second.source : nullptr;
    }

    bool IsSupported(uint32_t metricId) const
    {
        auto it = metrics.find(metricId);
        return it != metrics.end() && it->second.supported;
    }

    // Fills in metrics this snapshot doesn't already have from a backup source —
    // used when a primary provider can't supply a metric a fallback provider can.
    // The value carries its own source across, so a merged reading still names the
    // provider that actually produced it rather than the one that was asked first.
    void MergeMissing(const Snapshot& backup)
    {
        for (const auto& [id, mv] : backup.metrics)
        {
            if (mv.supported && !IsSupported(id))
                metrics[id] = mv;
        }
    }

    // Lets a secondary provider WIN for one metric, which MergeMissing deliberately
    // cannot. Used where a provider is not a fallback but the authority: NVAPI owns the
    // NVIDIA clocks because it is what every other tool on the machine reads.
    void Override(uint32_t metricId, const Snapshot& other)
    {
        if (other.IsSupported(metricId))
            metrics[metricId] = other.metrics.at(metricId);
    }
};
