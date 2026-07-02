#include "Modules/GPU/GpuResolver.h"

#include "Providers/Nvml/NvmlProvider.h"
#include "Providers/Nvapi/NvapiProvider.h"
#include "Providers/Adlx/AdlxProvider.h"
#include "Providers/Adl2/Adl2Provider.h"

#include "Utils/Debug.h"

bool GpuResolver::Initialize()
{
    // NVIDIA: NVML → NVAPI fallback
    {
        auto nvml = std::make_unique<NvmlProvider>();
        if (nvml->Initialize() && nvml->GetDeviceCount() > 0)
        {
            uint32_t count = nvml->GetDeviceCount();
            for (uint32_t i = 0; i < count; ++i)
                m_devices.push_back({ nvml.get(), i, GpuVendor::Nvidia });

            LOG_STARTUP(L"GpuResolver: NVML initialized (%u device(s))", count);
            m_providers.push_back(std::move(nvml));
        }
        else
        {
            auto nvapi = std::make_unique<NvapiProvider>();
            if (nvapi->Initialize() && nvapi->GetDeviceCount() > 0)
            {
                uint32_t count = nvapi->GetDeviceCount();
                for (uint32_t i = 0; i < count; ++i)
                    m_devices.push_back({ nvapi.get(), i, GpuVendor::Nvidia });

                LOG_STARTUP(L"GpuResolver: NVAPI fallback initialized (%u device(s))", count);
                m_providers.push_back(std::move(nvapi));
            }
        }
    }

    // AMD: ADLX → ADL2 fallback
    {
        auto adlx = std::make_unique<AdlxProvider>();
        if (adlx->Initialize() && adlx->GetDeviceCount() > 0)
        {
            uint32_t count = adlx->GetDeviceCount();
            for (uint32_t i = 0; i < count; ++i)
                m_devices.push_back({ adlx.get(), i, GpuVendor::AMD });

            LOG_STARTUP(L"GpuResolver: ADLX initialized (%u device(s))", count);
            m_providers.push_back(std::move(adlx));
        }
        else
        {
            auto adl2 = std::make_unique<Adl2Provider>();
            if (adl2->Initialize() && adl2->GetDeviceCount() > 0)
            {
                uint32_t count = adl2->GetDeviceCount();
                for (uint32_t i = 0; i < count; ++i)
                    m_devices.push_back({ adl2.get(), i, GpuVendor::AMD });

                LOG_STARTUP(L"GpuResolver: ADL2 fallback initialized (%u device(s))", count);
                m_providers.push_back(std::move(adl2));
            }
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
}

bool GpuResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_devices.size())
        return false;

    auto& dev = m_devices[deviceIndex];
    return dev.provider->GetString(metricId, dev.localIndex, out);
}
