#pragma once

#include <cstdint>
#include <unordered_map>

struct MetricValue
{
    double value     = 0.0;
    bool   supported = false;
};

struct Snapshot
{
    std::unordered_map<uint32_t, MetricValue> metrics;

    void Set(uint32_t metricId, double value)
    {
        metrics[metricId] = { value, true };
    }

    double Get(uint32_t metricId) const
    {
        auto it = metrics.find(metricId);
        if (it == metrics.end() || !it->second.supported)
            return -1.0;

        return it->second.value;
    }

    bool IsSupported(uint32_t metricId) const
    {
        auto it = metrics.find(metricId);
        return it != metrics.end() && it->second.supported;
    }

    // Fills in metrics this snapshot doesn't already have from a backup source —
    // used when a primary provider can't supply a metric a fallback provider can.
    void MergeMissing(const Snapshot& backup)
    {
        for (const auto& [id, mv] : backup.metrics)
        {
            if (mv.supported && !IsSupported(id))
                metrics[id] = mv;
        }
    }
};
