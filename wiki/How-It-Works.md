# How It Works

One DLL, five layers, no shared mutable state between the SDK code and the Rainmeter
boundary. Each layer only knows about the one below it.

```
Plugin  →  Core  →  Modules  →  Resolvers  →  Providers
```

| Layer | Location | Job | Knows about |
|---|---|---|---|
| **Plugin** | `source/Plugin/` | Rainmeter entry points (`Initialize`/`Reload`/`Update`/`Finalize`/`GetString`). Reads measure options, hands them to Core. | Rainmeter only — no hardware |
| **Core** | `source/Core/` | `HardwareCore` singleton + poller. Deduplicates the gather so each category is read once per tick; `GetValue`/`GetString` are pure cache reads. | Modules only — no SDKs |
| **Modules** | `source/Modules/<Cat>/` | One per category. Owns the per-device `Snapshot[]` cache; answers metric queries from it. | Its Resolver — no SDKs |
| **Resolvers** | `source/Modules/<Cat>/` | The vendor-aware priority chain. At init, assigns each device to the provider that wins for it. | Providers |
| **Providers** | `source/Providers/<SDK>/` | Leaf nodes. **The only place an SDK is touched.** | One SDK each |

## The read path, one tick

1. Rainmeter calls `Update` on a measure → `HardwareCore::GetValue(handle)`.
2. Core makes sure the owning module has gathered this tick (once, shared across every
   measure of that category), then returns the cached value. No SDK call happens inside
   `GetValue` — gathering and reading are separated.
3. A module gathers by asking, per device, the provider the Resolver picked for it, which
   fills a `Snapshot` (a `metricId → {value, supported}` map).

`supported = false` is **not** `value = 0`. A metric the hardware genuinely can't report
is marked unsupported and surfaces as `-1`; a real zero reading stays `0`. See
**[Quirks & Troubleshooting](Quirks-and-Troubleshooting)**.

## Polling model

By default a category is gathered in step with Rainmeter's `Update=` tick. Set
[`UpdateOverride=<ms>`](Configuring-Measures#updateoverride--default-0-follow-rainmeter)
on a measure and that category is instead polled on a background thread at that rate,
independent of the skin — so a fast network readout can live under a slow skin, and the
gather never blocks Rainmeter's UI thread.

## GPU vendor fallback

GPU is the only multi-provider category. NVIDIA and AMD are enumerated **simultaneously** —
whichever card(s) are present show up. Each vendor has a primary provider and a backup:

- **NVIDIA:** NVML *(primary)* → NVAPI *(backup)*
- **AMD:** ADLX *(primary)* → ADL2 *(legacy backup)*

Fallback is **per metric, not per card**: if the primary can't supply a given value, the
vendor's backup fills just that one. For `Name` and `DriverVersion` there's a final
Windows DXGI/registry last resort so those are never blank. A blank cell in the
[Metrics Reference](Metrics-Reference) support tables is a real SDK/hardware ceiling, not a
missing fallback.

The other categories (CPU, Memory, Network, Storage, Ping) each have a single Windows-API
provider — no vendor chain.

## Why no driver

Every reading comes from a user-mode source: vendor SDKs (NVML/NVAPI/ADLX/ADL2) and
Windows APIs (WMI, PDH, Win32, IP Helper, ICMP). Values that only exist behind Model-Specific
Registers or vendor SMU telemetry — real per-core CPU voltage, true Tctl/Tdie temperature —
would require a signed kernel driver and a service to load it. That's a permanent scope
boundary, not a TODO. Such metrics report `-1`. See
**[Quirks & Troubleshooting](Quirks-and-Troubleshooting#no-cpu-temperature-or-real-voltage-without-a-driver)**.
