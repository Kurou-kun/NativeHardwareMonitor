#pragma once

#include "Core/IModule.h"
#include "Core/IProvider.h"
#include "Types/Category.h"
#include "Types/MetricParser.h"
#include "Types/Snapshot.h"
#include "Utils/Debug.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

// A category is a provider plus one snapshot per device. That is all nine of the
// ten categories ever were, and each of them used to say it in a Module class and a
// Resolver class of its own - files that diffed to zero lines against each other once
// the type names were normalised, differing only in which provider they named and what
// noun their startup log used.
//
// GPU is the one category with real resolution to do (five providers, a per-metric merge,
// Windows counters behind all of them). `GpuResolver` is an IProvider itself, so it drops
// in here like any other source and keeps its logic - which is why this class needs no
// template and no hook: composition, not inheritance per category.
class Module final : public IModule
{
public:
    // countNoun: the unit a startup log should count in ("drive(s)", "adapter(s)").
    // nullptr for a category whose device count says nothing worth logging.
    Module(Category category, std::unique_ptr<IProvider> provider,
           const wchar_t* providerName, const wchar_t* countNoun = nullptr)
        : m_category(category)
        , m_provider(std::move(provider))
        , m_providerName(providerName)
        , m_countNoun(countNoun)
    {
    }

    bool Initialize() override
    {
        if (m_initialized)
            return true;

        if (!m_provider || !m_provider->Initialize())
        {
            LOG_STARTUP(L"%s: %s initialization failed",
                CategoryName(m_category).c_str(), m_providerName);
            return false;
        }

        if (m_countNoun)
            LOG_STARTUP(L"%s: %s initialized (%u %s)", CategoryName(m_category).c_str(),
                m_providerName, m_provider->GetDeviceCount(), m_countNoun);
        else
            LOG_STARTUP(L"%s: %s initialized", CategoryName(m_category).c_str(), m_providerName);

        // Ping starts with no targets at all and grows as hosts register, so this is a
        // starting size rather than a final one - see EnsureSnapshots.
        m_snapshots.resize(m_provider->GetDeviceCount());
        m_initialized = true;

        return true;
    }

    void GatherAll() override
    {
        if (!m_initialized)
            return;

        uint32_t count = m_provider->GetDeviceCount();
        EnsureSnapshots(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            m_snapshots[i].currentSource = m_providerName;
            m_provider->GatherSnapshot(i, m_snapshots[i]);
        }
    }

    void GatherDevices(const std::vector<uint32_t>& devices) override
    {
        if (!m_initialized)
            return;

        if (devices.empty())
        {
            GatherAll();
            return;
        }

        uint32_t count = m_provider->GetDeviceCount();
        EnsureSnapshots(count);

        for (uint32_t i : devices)
        {
            if (i < count)
            {
                m_snapshots[i].currentSource = m_providerName;
                m_provider->GatherSnapshot(i, m_snapshots[i]);
            }
        }
    }

    double GetValue(uint32_t metricId, uint32_t deviceIndex) override
    {
        if (deviceIndex >= m_snapshots.size())
            return -2.0;

        return m_snapshots[deviceIndex].Get(metricId);
    }

    bool GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override
    {
        return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
    }

    // GPU overwrites this per sub-provider inside GpuResolver; every other category keeps
    // the name stamped above, which is the only provider it has.
    const wchar_t* GetSource(uint32_t metricId, uint32_t deviceIndex) override
    {
        if (deviceIndex >= m_snapshots.size())
            return nullptr;

        return m_snapshots[deviceIndex].SourceOf(metricId);
    }

    uint32_t GetDeviceCount() const override
    {
        return m_provider ? m_provider->GetDeviceCount() : 0;
    }

    // Ping only: a host string becomes a device slot, created on first sight. The
    // snapshot vector has to cover it before the next gather writes there, and this runs
    // on Rainmeter's thread under the module mutex (see HardwareCore::RegisterMeasure).
    uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) override
    {
        if (!m_provider)
            return 0;

        uint32_t index = m_provider->ResolveTarget(host, intervalMs);
        EnsureSnapshots(index + 1);

        return index;
    }

private:
    // Never shrinks: a snapshot holds the last reading for a device, and a provider whose
    // count drops should not drop the values a measure is still reading.
    void EnsureSnapshots(size_t needed)
    {
        if (m_snapshots.size() < needed)
            m_snapshots.resize(needed);
    }

    Category                   m_category;
    std::unique_ptr<IProvider> m_provider;
    const wchar_t*             m_providerName;
    const wchar_t*             m_countNoun;

    std::vector<Snapshot>      m_snapshots;
    bool                       m_initialized = false;
};
