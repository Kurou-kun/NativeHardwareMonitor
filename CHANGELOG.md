# Changelog

All notable changes to NativeHardwareMonitor are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/);
this project uses a four-part `MAJOR.MINOR.PATCH.BUILD` version in the DLL resource.

## [2.0.0] — Unreleased

A ground-up rewrite. v1.x was a 5-category plugin built on a "Backend" class
hierarchy; v2 replaces it with a layered Module → Resolver → Provider
architecture, doubles the category count to ten, adds a third GPU vendor, and
introduces Lua support, metric aliases, and a diagnostic dump. Everything is
still a single driverless DLL — no kernel driver, no service, no elevated install.

### Added

**New categories (5 → 10):**
- **Ping** — ICMP echo per host: `Rtt`, `PacketLoss`, `MinRtt`/`MaxRtt`/`AvgRtt`,
  `Jitter`, `Ttl`, `PacketsSent`/`PacketsReceived`, `ResolvedIp`, `Status`.
  Background per-target polling; targets resolved from a `Host=` string.
- **Battery** — `ChargeLevel`, `Charging`, `AcOnline`, `TimeToEmpty`, `Rate`,
  `Voltage`, `RemainingCapacity`, `FullChargeCapacity`, `DesignCapacity`,
  `WearLevel`, `CycleCount`, `Status`, `Chemistry`, `Manufacturer`,
  `SerialNumber`, `DeviceName`. Handles the no-battery (desktop) case.
- **System** — `Uptime`, `ProcessCount`, `ThreadCount`, `HandleCount`, `OsName`,
  `OsVersion`, `OsBuild`, `Hostname`, `UserName`, `BootTime`.
- **Motherboard** — `Manufacturer`, `Product`, `SerialNumber`, `BiosVersion`,
  `BiosDate`, `SystemManufacturer`, `SystemProduct` (WMI CIM).
- **Display** — multi-monitor: `Width`, `Height`, `RefreshRate`, `BitsPerPixel`,
  `Primary`, `Count`, `Name`, `DeviceName`, `Resolution`.

**Intel GPU support (3rd GPU vendor):**
- New IGCL provider via `ControlLib.dll` (ships with the Intel Arc/Xe driver —
  driverless). NVIDIA, AMD, and Intel are now enumerated simultaneously.
- Intel metrics: usage, core/memory clock, max clock, GPU/VRAM temperature,
  power, total board power, power limit, voltage, fan (RPM/%), VRAM used/total,
  PCIe link gen/width, plus `Name`/`PciDeviceId`/`VbiosVersion`/`ThrottleReasons`.
- Power and utilization are derived from IGCL's monotonic counters (per-tick
  delta), seeded on the first read.

**Second-source GPU providers (per-metric fallback):**
- NVIDIA gained **NVAPI** as a backup to NVML; AMD gained **ADL2** as a legacy
  backup to ADLX. A metric the primary can't supply is filled from the backup,
  then from a Windows DXGI/registry last resort for `Name`/`DriverVersion`.

**Expanded metrics on existing categories:**
- **GPU** string metrics (`Name`, `DriverVersion`, `VbiosVersion`,
  `PciDeviceId`, `ThrottleReasons`) and more numerics (`HotspotTemperature`,
  `MemoryTemperature`, `IntakeTemperature`, `PowerLimit`, `TotalBoardPower`,
  `MaxCoreClock`, `PcieLinkGen`/`Width`, `EncoderUsage`, `DecoderUsage`,
  `PerfState`, `FanSpeedRPM`).
- **CPU** topology and identity: `MaxClock`, `Name`, `Vendor`, `Identifier`,
  `MicrocodeVersion`, `CoreCount`, `ThreadCount`, `CacheL1`/`L2`/`L3`,
  `Architecture`.
- **Memory** hardware metadata: `Cached`, `Speed`, `MemoryType`, `Manufacturer`,
  `PartNumber`, `ModuleCount`.
- **Network** beyond throughput: totals, `ReceiveLinkSpeed`, packet/error/discard
  counters, `Mtu`, `Alias`, `Description`, `PhysicalAddress`, `ConnectionStatus`,
  and Wi-Fi (`WifiSignal`, `WifiRxRate`/`TxRate`, `Ssid`, `WifiRadioType`).
- **Storage** activity + volume info: `ReadSpeed`/`WriteSpeed`, `QueueLength`,
  `BusyPercent`, `ReadsPerSec`/`WritesPerSec`, `VolumeLabel`, `FileSystem`,
  `DriveType` (unlabeled volumes → "Local Disk").

**Other additions:**
- **Lua support** — plugin values are readable from Rainmeter Lua (5.1); number,
  string, and sentinel values cross the plugin→Lua boundary intact.
- **Short names / aliases** — categories and metrics accept alias forms
  (e.g. `Category=gpu` or `graphics`; `Metric=temp` for `Temperature`).
- **`Debug=2` metric dump** — writes
  `%TEMP%\NativeHardwareMonitor\<Category>_dev<N>.txt` (throttled ~2 s) listing
  every metric for that category+device, with `unsupported`/`error` annotations.
  Built to let owners of hardware the author can't test (e.g. Intel GPUs) send
  back a values file for verification.
- **`UpdateOverride`** per-measure poll interval and a background `PollerThread`
  for dynamic polling.
- **Snapshot value semantics** — `-1` = hardware/SDK unsupported, `-2` = plugin
  error, `0`/positive = a real reading (consistent across every category).
- GitHub **wiki** (Home, Installation, Configuring-Measures, Metrics-Reference,
  How-It-Works, Quirks-and-Troubleshooting) and this changelog.

### Changed

- **Architecture rewrite:** the v1 `*Backend` / `ICategoryBackend` / `BaseBackend`
  hierarchy is replaced by a layered **`IModule` → Resolver → `IProvider`** design
  with a shared `Snapshot`. GPU is the only multi-provider category; the rest have
  a single Windows-API provider.
- **SDK vendoring reorganized** from `external/<vendor>/…` to `sdk/<sdk>/`
  (`sdk/nvml`, `sdk/nvapi`, `sdk/adlx`, `sdk/adl`, `sdk/igcl`, `sdk/rainmeter`).
- **Versioning** moved from a `Version.h` macro scheme to the `.rc` resource in
  the Rainmeter-recognized format (`ProductName="Rainmeter"` marker); bumped to
  `2.0.0.0`.
- Shared `Wmi::` helper (`source/Utils/WmiUtil`) now backs all WMI reads
  (CPU/Memory/Motherboard/System/Battery).

### Removed

- **MSR driver stub** (`Utils/MsrDriver.*`) — the project will never ship a kernel
  driver or service. MSR/SMU-only metrics (real per-core CPU voltage, Tctl/Tdie
  temperature) are permanently out of scope.
- The v1 **Dummy** category/backend and `Utils/TimeUtils`.

### Fixed

- Network per-tick lag from a redundant interface-table fetch and per-adapter WLAN
  RPCs (Wi-Fi detection now precomputed; interface table cached per cycle).
- `RPC_E_CHANGED_MODE` WMI COM-init conflict in the CPU and Memory providers.
- Storage PDH "collect twice" requirement (first sample was always zero).
- GPU voltage unit consistency — IGCL now reports millivolts to match ADLX.

## [1.0.2] — 2026-03-15

Last release of the v1 line. Five categories (CPU, GPU, Memory, Network, Storage)
on the original Backend architecture; GPU via NVIDIA (NVML) and AMD (ADLX) only.

## [1.0.1] — 2026-03-04

Incremental fixes on the v1 line.

## [1.0.0] — 2026-03-04

Initial release.
