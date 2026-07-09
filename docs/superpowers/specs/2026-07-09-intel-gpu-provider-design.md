# Intel GPU Provider (IGCL) — Design

**Date:** 2026-07-09
**Status:** Approved design, pending implementation plan
**Reference:** Intel Graphics Control Library — https://intel.github.io/drivers.gpu.control-library/

## Goal

Add Intel GPU telemetry to the GPU module, closing the last vendor gap
(NVIDIA and AMD are already supported). Driverless, consistent with the
existing provider pattern, no new metric enum values unless a sensor
genuinely needs one.

## Why IGCL

IGCL (Intel Graphics Control Library) loads `ControlLib.dll`, which ships
with the installed Intel graphics driver — user-mode, no kernel component.
This satisfies the project's permanent no-driver policy and mirrors the
existing `LoadLibrary` + `GetProcAddress` shape used by NVML/NVAPI/ADLX/ADL2.
It is the same source HWiNFO and LibreHardwareMonitor use for Intel Arc and
Xe/UHD iGPU telemetry. Level Zero Sysman was rejected: heavier, compute-
oriented, and requires the L0 loader present.

## Architecture

Mirrors the existing single-vendor providers exactly.

- **New files:** `source/Providers/Igcl/IgclProvider.{h,cpp}`, implementing
  `IProvider` (`Initialize` / `GetDeviceCount` / `GatherSnapshot` / `GetString`).
- **Vendored header:** `sdk/igcl/igcl_api.h` (+ `ctl_api.h` if the header
  splits) taken verbatim from `intel/drivers.gpu.control-library`, the same
  way `sdk/nvml/nvml.h` is vendored. Header-only; no import lib — the DLL is
  resolved at runtime.
- **DLL loaded at runtime:** `ControlLib.dll`. Absent on machines without an
  Intel driver (including this dev machine), so `Initialize()` must fail
  cleanly to "no device" — identical to how ADLX degrades today.
- **Enum:** `GpuVendor` gains `Intel`.
- **Resolver wiring:** `GpuResolver::Initialize()` gets an Intel block after
  the AMD block. **Single API — no per-metric backup provider** (NVIDIA/AMD
  pair two SDKs; Intel has only IGCL). Intel `DeviceEntry` rows therefore have
  no `backupProvider`.
- **Windows fallback:** add `PCI_VENDOR_INTEL = 0x8086` in `GpuResolver` so
  the existing `m_winFallback.GetName` / `GetDriverVersion` last-resort path
  covers Intel devices when IGCL omits those strings.

## IGCL call sequence

1. `ctlInit(&initArgs, &hApi)` — one API handle.
2. `ctlEnumerateDevices(hApi, &count, handles)` — array of
   `ctl_device_adapter_handle_t`. One `DeviceEntry` per handle.
3. Per device, once: `ctlGetDeviceProperties(handle, &props)` →
   `ctl_device_adapter_properties_t` (`name`, `pci_vendor_id`,
   `pci_device_id`, `driver_version` as `uint64_t`).
4. Per tick: `ctlPowerTelemetryGet(handle, &telemetry)` →
   `ctl_power_telemetry_t`. Every field is a `ctl_oc_telemetry_item_t` carrying
   a `bSupported` flag plus `{ units, type, value }` — so unsupported sensors are
   self-declaring and map straight to the -1 "unsupported" semantics.

Field list confirmed verbatim against IGCL 1.1 spec: `timeStamp`,
`gpuEnergyCounter`, `gpuVoltage`, `gpuCurrentClockFrequency`,
`gpuCurrentTemperature`, `globalActivityCounter`,
`renderComputeActivityCounter`, `mediaActivityCounter`, the five
`gpu*Limited` throttle bools, `vramEnergyCounter`, `vramVoltage`,
`vramCurrentClockFrequency`, `vramCurrentTemperature`, VRAM bandwidth
counters, and `fanSpeed[]`.

## Metric mapping

Onto existing `GpuMetric` values (no new enum members):

| GpuMetric        | IGCL source                                             |
|------------------|---------------------------------------------------------|
| Usage            | `globalActivityCounter` (Δcounter / Δtimestamp)         |
| CoreClock        | `gpuCurrentClockFrequency`                              |
| MemoryClock      | `vramCurrentClockFrequency`                             |
| Temperature      | `gpuCurrentTemperature`                                 |
| MemoryTemperature| `vramCurrentTemperature`                               |
| VramUsed/Total   | memory telemetry fields (if populated)                  |
| FanSpeed / RPM   | `fanSpeed[]`                                             |
| Power            | `gpuEnergyCounter` (ΔJoules / Δtimestamp → Watts)       |
| Voltage          | `gpuVoltage`                                            |
| Name             | `ctl_device_adapter_properties_t.name`                  |
| DriverVersion    | `driver_version` (or Windows fallback)                  |
| PciDeviceId      | `pci_device_id`                                          |

Any sensor the hardware does not populate (`bSupported == false`) is left
unset → module reports -1. Common on iGPU: fan and discrete power rails.

## The one real nuance: counters, not instantaneous values

IGCL reports **power as a cumulative energy counter (joules)** and
**utilization as a cumulative activity counter**, alongside a `timeStamp`.
Instantaneous Watts / percent require a per-tick delta:

```
value = (counter_now - counter_prev) / (timestamp_now - timestamp_prev)
```

The provider stores the previous counter+timestamp per device and seeds on
the first read (first tick returns -1 / unset for these two, real values from
the second tick on). This is the calibration/tuning point and the only
non-trivial logic. Marked with a `// ponytail:` comment.

## Deferred (reachable, left out of first pass for minimalism)

- **ThrottleReasons** — reachable via the five `gpu*Limited` bool flags
  (power/temperature/current/voltage/utilization). Cheap to add later; kept
  out of the first pass to avoid string-formatting scope creep. Add on request
  or once the target machine's other sensors are confirmed live.

## Out of scope

PcieLinkGen/Width, EncoderUsage, DecoderUsage, PerfState, HotspotTemperature
— not exposed by `ctl_power_telemetry_t`.

## Verification

No Intel GPU on the dev machine (RTX 4060 + Ryzen 9700X), so:

- **Here:** compile-verify, confirm the DLL-absent path degrades to "no Intel
  device" without disturbing the NVIDIA path, deploy.
- **Target work machine (has Intel GPU):** live sensor read-back in Rainmeter.

## Testing

One runnable self-check on the counter→rate delta math (seed behaviour + a
two-sample rate calculation). Everything else is guarded straight reads —
no test needed per project convention.
