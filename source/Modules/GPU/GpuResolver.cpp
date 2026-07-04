#include "Modules/GPU/GpuResolver.h"

#include "Providers/Nvml/NvmlProvider.h"
#include "Providers/Nvapi/NvapiProvider.h"
#include "Providers/Adlx/AdlxProvider.h"
#include "Providers/Adl2/Adl2Provider.h"

#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

static constexpr uint32_t PCI_VENDOR_NVIDIA = 0x10DE;
static constexpr uint32_t PCI_VENDOR_AMD    = 0x1002;

bool GpuResolver::Initialize()
{
    // NVIDIA: NVML primary, NVAPI kept alive as a per-metric backup —
    // some metrics (or whole feature sets on older GPUs/drivers) are only
    // available through one of the two APIs.
    {
        auto nvml  = std::make_unique<NvmlProvider>();
        auto nvapi = std::make_unique<NvapiProvider>();

        bool nvmlOk  = nvml->Initialize()  && nvml->GetDeviceCount()  > 0;
        bool nvapiOk = nvapi->Initialize() && nvapi->GetDeviceCount() > 0;

        if (nvmlOk)
        {
            uint32_t count = nvml->GetDeviceCount();
            for (uint32_t i = 0; i < count; ++i)
            {
                DeviceEntry entry{ nvml.get(), i, GpuVendor::Nvidia };
                if (nvapiOk && i < nvapi->GetDeviceCount())
                {
                    entry.backupProvider   = nvapi.get();
                    entry.backupLocalIndex = i;
                }
                m_devices.push_back(entry);
            }

            LOG_STARTUP(L"GpuResolver: NVML initialized (%u device(s))%s", count,
                        nvapiOk ? L", NVAPI available as metric backup" : L"");

            m_providers.push_back(std::move(nvml));
            if (nvapiOk)
                m_providers.push_back(std::move(nvapi));
        }
        else if (nvapiOk)
        {
            uint32_t count = nvapi->GetDeviceCount();
            for (uint32_t i = 0; i < count; ++i)
                m_devices.push_back({ nvapi.get(), i, GpuVendor::Nvidia });

            LOG_STARTUP(L"GpuResolver: NVAPI fallback initialized (%u device(s))", count);
            m_providers.push_back(std::move(nvapi));
        }
    }

    // AMD: ADLX primary, ADL2 kept alive as a per-metric backup
    {
        auto adlx = std::make_unique<AdlxProvider>();
        auto adl2 = std::make_unique<Adl2Provider>();

        bool adlxOk = adlx->Initialize() && adlx->GetDeviceCount() > 0;
        bool adl2Ok = adl2->Initialize() && adl2->GetDeviceCount() > 0;

        if (adlxOk)
        {
            uint32_t count = adlx->GetDeviceCount();
            for (uint32_t i = 0; i < count; ++i)
            {
                DeviceEntry entry{ adlx.get(), i, GpuVendor::AMD };
                if (adl2Ok && i < adl2->GetDeviceCount())
                {
                    entry.backupProvider   = adl2.get();
                    entry.backupLocalIndex = i;
                }
                m_devices.push_back(entry);
            }

            LOG_STARTUP(L"GpuResolver: ADLX initialized (%u device(s))%s", count,
                        adl2Ok ? L", ADL2 available as metric backup" : L"");

            m_providers.push_back(std::move(adlx));
            if (adl2Ok)
                m_providers.push_back(std::move(adl2));
        }
        else if (adl2Ok)
        {
            uint32_t count = adl2->GetDeviceCount();
            for (uint32_t i = 0; i < count; ++i)
                m_devices.push_back({ adl2.get(), i, GpuVendor::AMD });

            LOG_STARTUP(L"GpuResolver: ADL2 fallback initialized (%u device(s))", count);
            m_providers.push_back(std::move(adl2));
        }
    }

    if (m_devices.empty())
        LOG_ERROR(L"GpuResolver: no GPU provider initialized successfully");

    return !m_devices.empty();
}

uint32_t GpuResolver::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}

void GpuResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex >= m_devices.size())
        return;

    auto& dev = m_devices[deviceIndex];
    dev.provider->GatherSnapshot(dev.localIndex, snap);

    if (dev.backupProvider)
    {
        Snapshot backup;
        dev.backupProvider->GatherSnapshot(dev.backupLocalIndex, backup);
        snap.MergeMissing(backup);
    }
}

bool GpuResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_devices.size())
        return false;

    auto& dev = m_devices[deviceIndex];

    if (dev.provider->GetString(metricId, dev.localIndex, out))
        return true;

    if (dev.backupProvider && dev.backupProvider->GetString(metricId, dev.backupLocalIndex, out))
        return true;

    // Windows-side last resort — only covers Name/DriverVersion, and only actually
    // matters for the ADL2-only path (no other SDK gap it needs to plug today).
    auto metric = static_cast<GpuMetric>(metricId);
    if (metric != GpuMetric::Name && metric != GpuMetric::DriverVersion)
        return false;

    uint32_t pciVendorId = dev.vendor == GpuVendor::Nvidia ? PCI_VENDOR_NVIDIA
                          : dev.vendor == GpuVendor::AMD   ? PCI_VENDOR_AMD
                          : 0;
    if (pciVendorId == 0)
        return false;

    return metric == GpuMetric::Name
        ? m_winFallback.GetName(pciVendorId, dev.localIndex, out)
        : m_winFallback.GetDriverVersion(pciVendorId, dev.localIndex, out);
}
