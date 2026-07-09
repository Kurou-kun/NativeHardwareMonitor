#include "Providers/Igcl/IgclProvider.h"
#include "Types/GpuMetric.h"
#include "Utils/Debug.h"

#include <cstring>

static constexpr uint32_t PCI_VENDOR_INTEL = 0x8086;

// Read a telemetry item's value as a double regardless of its declared data type.
static double ItemVal(const ctl_oc_telemetry_item_t& it)
{
    switch (it.type)
    {
    case CTL_DATA_TYPE_INT8:   return it.value.data8;
    case CTL_DATA_TYPE_UINT8:  return it.value.datau8;
    case CTL_DATA_TYPE_INT16:  return it.value.data16;
    case CTL_DATA_TYPE_UINT16: return it.value.datau16;
    case CTL_DATA_TYPE_INT32:  return it.value.data32;
    case CTL_DATA_TYPE_UINT32: return it.value.datau32;
    case CTL_DATA_TYPE_INT64:  return static_cast<double>(it.value.data64);
    case CTL_DATA_TYPE_UINT64: return static_cast<double>(it.value.datau64);
    case CTL_DATA_TYPE_FLOAT:  return it.value.datafloat;
    case CTL_DATA_TYPE_DOUBLE: return it.value.datadouble;
    default:                   return 0.0;
    }
}

bool IgclProvider::Initialize()
{
    m_module = LoadLibraryW(L"ControlLib.dll");
    if (!m_module)
    {
        LOG_STARTUP(L"IgclProvider: ControlLib.dll not found (no Intel driver)");
        return false;
    }

    if (!LoadFunctions())
    {
        LOG_STARTUP(L"IgclProvider: required functions missing");
        Shutdown();
        return false;
    }

    ctl_init_args_t args{};
    args.Size       = sizeof(args);
    args.Version    = 0;
    args.AppVersion = CTL_MAKE_VERSION(CTL_IMPL_MAJOR_VERSION, CTL_IMPL_MINOR_VERSION);
    // ponytail: flags=0 works on current drivers; if ctlPowerTelemetryGet reports
    // nothing on the target, flip CTL_INIT_FLAG_USE_LEVEL_ZERO (needs the L0 loader).
    args.flags = 0;

    if (m_Init(&args, &m_api) != CTL_RESULT_SUCCESS || !m_api)
    {
        LOG_STARTUP(L"IgclProvider: ctlInit failed");
        Shutdown();
        return false;
    }

    uint32_t count = 0;
    if (m_Enumerate(m_api, &count, nullptr) != CTL_RESULT_SUCCESS || count == 0)
    {
        LOG_STARTUP(L"IgclProvider: no adapters enumerated");
        Shutdown();
        return false;
    }

    std::vector<ctl_device_adapter_handle_t> handles(count, nullptr);
    if (m_Enumerate(m_api, &count, handles.data()) != CTL_RESULT_SUCCESS)
    {
        LOG_STARTUP(L"IgclProvider: adapter enumeration failed");
        Shutdown();
        return false;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        if (!handles[i])
            continue;

        ctl_device_adapter_properties_t props{};
        props.Size = sizeof(props);
        // pDeviceID buffer is optional for our needs; leave null (name/pci ids still fill).
        if (m_GetProperties(handles[i], &props) != CTL_RESULT_SUCCESS)
            continue;

        // ControlLib only enumerates Intel adapters, but filter defensively.
        if (props.pci_vendor_id != PCI_VENDOR_INTEL)
            continue;

        Device dev;
        dev.handle      = handles[i];
        dev.pciDeviceId = props.pci_device_id;
        dev.name        = std::string(props.name, ::strnlen(props.name, sizeof(props.name)));

        // Cache the first VRAM module handle so per-tick reads skip re-enumeration.
        if (m_EnumMemory && m_GetMemState)
        {
            uint32_t memCount = 0;
            if (m_EnumMemory(handles[i], &memCount, nullptr) == CTL_RESULT_SUCCESS && memCount > 0)
            {
                std::vector<ctl_mem_handle_t> mods(memCount, nullptr);
                if (m_EnumMemory(handles[i], &memCount, mods.data()) == CTL_RESULT_SUCCESS)
                    dev.memHandle = mods[0];
            }
        }

        m_devices.push_back(std::move(dev));
    }

    if (m_devices.empty())
    {
        LOG_STARTUP(L"IgclProvider: no Intel GPU among enumerated adapters");
        Shutdown();
        return false;
    }

    LOG_STARTUP(L"IgclProvider: initialized (%u Intel device(s))",
                static_cast<uint32_t>(m_devices.size()));
    return true;
}

bool IgclProvider::LoadFunctions()
{
#define LOAD(name, member) \
    member = reinterpret_cast<decltype(member)>(GetProcAddress(m_module, #name)); \
    if (!member) return false;

    LOAD(ctlInit,                m_Init)
    LOAD(ctlClose,               m_Close)
    LOAD(ctlEnumerateDevices,    m_Enumerate)
    LOAD(ctlGetDeviceProperties, m_GetProperties)
    LOAD(ctlPowerTelemetryGet,   m_GetTelemetry)

#undef LOAD

    // Optional — VRAM used/total. Skip silently if the runtime doesn't export them.
    m_EnumMemory  = reinterpret_cast<decltype(m_EnumMemory)>(GetProcAddress(m_module, "ctlEnumMemoryModules"));
    m_GetMemState = reinterpret_cast<decltype(m_GetMemState)>(GetProcAddress(m_module, "ctlMemoryGetState"));
    return true;
}

void IgclProvider::Shutdown()
{
    if (m_Close && m_api) m_Close(m_api);
    m_api = nullptr;
    if (m_module) { FreeLibrary(m_module); m_module = nullptr; }
}

uint32_t IgclProvider::GetDeviceCount() const
{
    return static_cast<uint32_t>(m_devices.size());
}

void IgclProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex >= m_devices.size())
        return;

    Device& dev = m_devices[deviceIndex];

    ctl_power_telemetry_t t{};
    t.Size = sizeof(t);
    if (m_GetTelemetry(dev.handle, &t) != CTL_RESULT_SUCCESS)
        return;

    auto set = [&](GpuMetric m, double v) { snap.Set(static_cast<uint32_t>(m), v); };

    // Instantaneous readings
    if (t.gpuCurrentClockFrequency.bSupported) set(GpuMetric::CoreClock,        ItemVal(t.gpuCurrentClockFrequency));
    if (t.vramCurrentClockFrequency.bSupported)set(GpuMetric::MemoryClock,      ItemVal(t.vramCurrentClockFrequency));
    if (t.gpuCurrentTemperature.bSupported)    set(GpuMetric::Temperature,      ItemVal(t.gpuCurrentTemperature));
    if (t.vramCurrentTemperature.bSupported)   set(GpuMetric::MemoryTemperature,ItemVal(t.vramCurrentTemperature));

    if (t.gpuVoltage.bSupported)
    {
        double v = ItemVal(t.gpuVoltage);
        if (t.gpuVoltage.units == CTL_UNITS_VOLTAGE_MILLIVOLTS) v /= 1000.0;
        set(GpuMetric::Voltage, v);
    }

    // Fan: first supported entry; RPM or percent depending on units.
    for (const auto& fan : t.fanSpeed)
    {
        if (!fan.bSupported) continue;
        if (fan.units == CTL_UNITS_ANGULAR_SPEED_RPM) set(GpuMetric::FanSpeedRPM, ItemVal(fan));
        else if (fan.units == CTL_UNITS_PERCENT)      set(GpuMetric::FanSpeed,    ItemVal(fan));
        break;
    }

    // Counter-derived: power (ΔJoules/Δs → W) and usage (Δbusy-seconds/Δs → %).
    // ponytail: monotonic counters — first tick only seeds; real values from the 2nd tick.
    if (t.timeStamp.bSupported && t.gpuEnergyCounter.bSupported && t.globalActivityCounter.bSupported)
    {
        double now      = ItemVal(t.timeStamp);
        double energy   = ItemVal(t.gpuEnergyCounter);
        double activity = ItemVal(t.globalActivityCounter);

        if (dev.seeded)
        {
            double dt = now - dev.prevTime;
            if (dt > 0.0)
            {
                set(GpuMetric::Power, (energy   - dev.prevEnergy)   / dt);
                set(GpuMetric::Usage, (activity - dev.prevActivity) / dt * 100.0);
            }
        }

        dev.prevTime     = now;
        dev.prevEnergy   = energy;
        dev.prevActivity = activity;
        dev.seeded       = true;
    }

    // VRAM used/total — separate call (not in the telemetry struct). Bytes.
    if (dev.memHandle)
    {
        ctl_mem_state_t mem{};
        mem.Size = sizeof(mem);
        if (m_GetMemState(dev.memHandle, &mem) == CTL_RESULT_SUCCESS && mem.size > 0)
        {
            set(GpuMetric::VramTotal, static_cast<double>(mem.size));
            set(GpuMetric::VramUsed,  static_cast<double>(mem.size - mem.free));
        }
    }
}

bool IgclProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex >= m_devices.size())
        return false;

    const Device& dev = m_devices[deviceIndex];
    auto metric = static_cast<GpuMetric>(metricId);

    switch (metric)
    {
    case GpuMetric::Name:
        if (dev.name.empty()) return false;
        out.assign(dev.name.begin(), dev.name.end());
        return true;

    case GpuMetric::PciDeviceId:
    {
        wchar_t buf[16];
        swprintf_s(buf, L"%04X:%04X", PCI_VENDOR_INTEL, dev.pciDeviceId & 0xFFFFu);
        out = buf;
        return true;
    }

    // DriverVersion: props.driver_version is an opaque uint64 — let GpuResolver's
    // Windows-registry fallback supply the human-readable string instead.
    default:
        return false;
    }
}
