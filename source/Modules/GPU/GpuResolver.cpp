#include "Modules/GPU/GpuResolver.h"

#include "Providers/Nvml/NvmlProvider.h"
#include "Providers/Nvapi/NvapiProvider.h"
#include "Providers/Adlx/AdlxProvider.h"
#include "Providers/Adl2/Adl2Provider.h"
#include "Providers/Igcl/IgclProvider.h"

#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

static constexpr uint32_t PCI_VENDOR_NVIDIA = 0x10DE;
static constexpr uint32_t PCI_VENDOR_AMD    = 0x1002;
static constexpr uint32_t PCI_VENDOR_INTEL  = 0x8086;

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
                entry.providerName = L"NVML";
                if (nvapiOk && i < nvapi->GetDeviceCount())
                {
                    entry.backupProvider   = nvapi.get();
                    entry.backupLocalIndex = i;
                    entry.backupName       = L"NVAPI";
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
            {
                DeviceEntry entry{ nvapi.get(), i, GpuVendor::Nvidia };
                entry.providerName = L"NVAPI";
                m_devices.push_back(entry);
            }

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
                entry.providerName = L"ADLX";
                if (adl2Ok && i < adl2->GetDeviceCount())
                {
                    entry.backupProvider   = adl2.get();
                    entry.backupLocalIndex = i;
                    entry.backupName       = L"ADL2";
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
            {
                DeviceEntry entry{ adl2.get(), i, GpuVendor::AMD };
                entry.providerName = L"ADL2";
                m_devices.push_back(entry);
            }

            LOG_STARTUP(L"GpuResolver: ADL2 fallback initialized (%u device(s))", count);
            m_providers.push_back(std::move(adl2));
        }
    }

    // Intel: IGCL only (single driverless API — no per-metric backup).
    {
        auto igcl = std::make_unique<IgclProvider>();
        if (igcl->Initialize() && igcl->GetDeviceCount() > 0)
        {
            uint32_t count = igcl->GetDeviceCount();
            for (uint32_t i = 0; i < count; ++i)
            {
                DeviceEntry entry{ igcl.get(), i, GpuVendor::Intel };
                entry.providerName = L"IGCL";
                m_devices.push_back(entry);
            }

            LOG_STARTUP(L"GpuResolver: IGCL initialized (%u device(s))", count);
            m_providers.push_back(std::move(igcl));
        }
    }

    AttachWindowsCounters();

    if (m_devices.empty())
        LOG_ERROR(L"GpuResolver: no GPU provider initialized successfully");

    return !m_devices.empty();
}

// Windows' own GPU counters, behind every vendor SDK and never in front of one. They
// only carry Usage, VramUsed and VramTotal, and MergeMissing means a vendor reading
// always wins — but on an AMD iGPU with no ADLX, or an Intel GPU with no ControlLib,
// they are the difference between three live metrics and three -1s.
//
// A device is matched to an adapter by vendor and by position within that vendor, the
// same way WinApiGpuInfo already resolves names, because DXGI and the vendor SDKs
// enumerate independently and share no index.
void GpuResolver::AttachWindowsCounters()
{
    if (m_devices.empty())
        return;

    auto win = std::make_unique<WinApiGpuProvider>();
    if (!win->Initialize() || win->GetDeviceCount() == 0)
        return;

    uint32_t nvidia = 0, amd = 0, intel = 0, matched = 0;
    for (auto& dev : m_devices)
    {
        uint32_t pciVendorId = 0, ordinal = 0;
        switch (dev.vendor)
        {
        case GpuVendor::Nvidia: pciVendorId = PCI_VENDOR_NVIDIA; ordinal = nvidia++; break;
        case GpuVendor::AMD:    pciVendorId = PCI_VENDOR_AMD;    ordinal = amd++;    break;
        case GpuVendor::Intel:  pciVendorId = PCI_VENDOR_INTEL;  ordinal = intel++;  break;
        default: continue;
        }

        int local = win->FindAdapter(pciVendorId, ordinal);
        if (local < 0)
            continue;

        dev.winProvider   = win.get();
        dev.winLocalIndex = static_cast<uint32_t>(local);
        ++matched;
    }

    LOG_STARTUP(L"GpuResolver: Windows GPU counters attached to %u of %u device(s)",
                matched, (uint32_t)m_devices.size());
    m_providers.push_back(std::move(win));
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

    snap.currentSource = dev.providerName;
    dev.provider->GatherSnapshot(dev.localIndex, snap);

    if (dev.backupProvider)
    {
        Snapshot backup;
        backup.currentSource = dev.backupName;
        dev.backupProvider->GatherSnapshot(dev.backupLocalIndex, backup);

        // NVAPI is not a fallback for the NVIDIA clocks - it is the authority, so it
        // overrides NVML rather than filling in behind it.
        //
        // Both APIs answer, and on a power-gated Optimus dGPU they answer differently:
        // NVML returns a value averaged over the interval (what HWiNFO calls the
        // effective clock, 9 MHz on an idle GTX 1650), NVAPI returns the clock the card
        // is set to (300 MHz) - and NVAPI is what MSI Afterburner, GPU-Z and HWiNFO all
        // read, so taking NVML there made this plugin the only tool disagreeing.
        //
        // This replaces a floor that ignored NVML below 100 MHz, which was wrong twice
        // over: it chose on the VALUE rather than on where the value came from, so it
        // could not tell a real 150 MHz from an averaged one (it passed the averaged one
        // through on the reporter's machine - forum p=246235), and a magnitude says
        // nothing about validity. NVML's reading is not discarded: it is published as
        // EffectiveClock, which is the metric it actually is.
        if (dev.vendor == GpuVendor::Nvidia)
        {
            snap.Override(static_cast<uint32_t>(GpuMetric::CoreClock),   backup);
            snap.Override(static_cast<uint32_t>(GpuMetric::MemoryClock), backup);
        }

        snap.MergeMissing(backup);
    }

    if (dev.winProvider)
    {
        Snapshot win;
        win.currentSource = L"Windows";
        dev.winProvider->GatherSnapshot(dev.winLocalIndex, win);
        snap.MergeMissing(win);
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
                          : dev.vendor == GpuVendor::Intel ? PCI_VENDOR_INTEL
                          : 0;
    if (pciVendorId == 0)
        return false;

    return metric == GpuMetric::Name
        ? m_winFallback.GetName(pciVendorId, dev.localIndex, out)
        : m_winFallback.GetDriverVersion(pciVendorId, dev.localIndex, out);
}
