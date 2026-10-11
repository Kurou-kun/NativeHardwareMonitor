# Changelog

All notable changes to NativeHardwareMonitor are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/);
this project uses a four-part `MAJOR.MINOR.PATCH.BUILD` version in the DLL resource.

## [2.1.8] — unreleased

### Fixed

- **Scrolling `sectionvariables.ini` quickly drove the device index to -2 and every
  reading to -1.** Reported by Yincognito. The skin clamps the device against the device
  count — `Clamp(#device# + 1, 0, [NHM:GPU(count)] - 1)` — and `count` could transiently
  come back **-1**, which turns that into `Clamp(x, 0, -2)`. The index then pointed at a
  device that cannot exist and everything read -1.

  `GetValue` returns a measure's last reported value when it cannot take the module mutex
  within 2 ms, and a *freshly registered* handle's last value is -1. Rapid scrolling
  registers a new handle per device touched while the poller is gathering, so that window
  is easy to hit. A device count needs no snapshot at all: the poller now publishes it to
  an atomic and `GetValue` answers it **before taking any lock**, so it is never negative
  — 0 until the first gather lands, then the real count. `DeviceCount()` reads the same
  atomic instead of waiting up to 50 ms for the mutex.

- **Every metric was unreadable for a full interval after a poller started.** The poll
  loop waited before its first callback, and the module is not even initialised until
  that callback runs — so a freshly loaded skin read -1 for up to a second, and a device
  count could still be unknown well after the skin appeared. It gathers first and waits
  afterwards.

### Changed

- **`sectionvariables.ini` floors its clamp bound**: `Max(0, [NHM:GPU(count)] - 1)`. The
  plugin no longer reports a negative count, but a skin should not turn a count of 0 into
  an index of -1 either, and this is the file people copy from.

### Checked, not changed

- **GPU clocks.** Reported as wrong against MSI Afterburner on an idle Optimus GTX 1650
  (9 MHz / 25 MHz here, 300 MHz / 405 MHz there). Both of our conversions are correct and
  that is now verified rather than assumed: `tools/nvapi_clock_check.py` calls
  `NvAPI_GPU_GetAllClockFrequencies` exactly as `NvapiProvider` does and agrees with
  nvidia-smi to the digit (720 MHz / 5001 MHz), and NVML's path tracks nvidia-smi across
  repeated samples. No scaling error exists in either path. What could not be reproduced
  here is the hardware state — a power-gated laptop dGPU on a 2021 driver — where NVML
  and NVAPI evidently disagree. If NVML is the one reporting the near-zero clock, the
  question is which source should win for clocks, not what the arithmetic is.

## [2.1.7] — unreleased

### Fixed

- **A skin whose only NHM measure sets `DynamicVariables=1` tore down and rebuilt its
  poller once a second.** Rainmeter calls `Reload` on *every update* for a measure with
  `DynamicVariables=1` — measured here at once per second per dynamic measure — and
  `Reload` unregistered the old handle before registering the new one. Since 2.1.5,
  unregistering the last measure stops every poller, so in a skin with exactly one NHM
  measure the plugin's measure map emptied for the instant between those two calls: the
  category's poller was destroyed and recreated each second, and only landed a gather by
  winning a race with the next tick. It did keep reading correct values on this machine —
  12 distinct readings in 12 seconds, no thread growth — but it was reading them on luck,
  and a slower gather or a slower machine loses that race and the skin goes stale.

  `Reload` now registers the new handle before releasing the old one, so at least one
  handle is registered throughout and the window never opens.

### Checked, not changed

- **`DynamicVariables=1` on measures works.** Reported in March (p=241552) as working
  "bar initially" — on v1, with `Sensor=`/`GpuIndex=`. Verified on v2 with `Device=#dev#`:
  switching the variable at runtime moves both a numeric and a string measure to the other
  GPU (`AMD Radeon(TM) Graphics`, 0%) while a static control measure stays on the first
  (29%), and switching back works too. The v2 rewrite fixed it; nothing here to repair.

## [2.1.6] — 2026-10-11

### Added

- **AMD without ADLX reads almost everything now.** `Adl2Provider` loaded three Overdrive5
  entry points and nothing else, so on a machine with no ADLX — an older driver, or an
  integrated GPU — usage and the two clocks worked and *every other metric read `-1`*. Not
  because the driver refused, but because nothing asked. It now also calls, each one
  optional so an older driver simply gets less:

  | call | fills |
  |---|---|
  | `ADL2_New_QueryPMLogData_Get` | temperature, hotspot, memory temp, fan, power, board power, voltage, PCIe link gen/width, GFX activity, both clocks, throttle reasons |
  | `ADL2_Adapter_VRAMUsage_Get` | VRAM in use |
  | `ADL2_Adapter_MemoryInfo2_Get` | VRAM total |
  | `ADL2_Adapter_VideoBiosInfo_Get` | VBIOS version |
  | `AdapterInfo.strPNPString` | PCI device id, parsed to `1002:13C0` |

  PMLog is the telemetry block GPU-Z and HWiNFO read, and it needs no `PMLog_Start` call.
  Each sensor carries its own `supported` flag, so a chip that reports fourteen of them
  gets fourteen and the rest stay `-1`. `Temperature` takes the first of EDGE, GFX, SOC,
  HOTSPOT that answers: a discrete card reports EDGE, the Raphael iGPU reports neither
  EDGE nor HOTSPOT but does report GFX and SOC.

  Measured on a Raphael iGPU, which went from four live metrics to fourteen: 53 °C,
  6.3 MB of 536.9 MB VRAM, 1339 mV, PCIe 3.0 x16, VBIOS `032.019.000.008`, `1002:13C0`.

- **`WinApiGpuProvider` — Windows' own GPU counters, behind every vendor SDK.** The
  `GPU Engine` and `GPU Adapter Memory` performance counters plus DXGI for the totals,
  matched to a device by adapter LUID. It supplies only `Usage`, `VramUsed` and
  `VramTotal`, it is merged last so a vendor reading always wins, and it answers on
  hardware no SDK covers — an AMD iGPU with no ADLX, an Intel GPU with no ControlLib,
  any card whose SDK is missing or too old. Usage is the busiest engine *type*, summed
  across processes, which is the number Task Manager shows; adding every engine together
  would read well over 100%.

  Temperature, power, voltage and fan have no Windows-side source without a kernel
  driver, so they are not offered here. That remains the line this plugin does not cross.

## [2.1.5] — 2026-10-11

### Fixed

- **Rainmeter crashed when the last NativeHardwareMonitor measure went away.** Deactivating
  the only skin that used the plugin — or refreshing until no NHM measure was left — killed
  Rainmeter outright. Windows Error Reporting named the module exactly:

  ```
  P1: Rainmeter.exe                            P4: NativeHardwareMonitor.dll_unloaded
  P5: 2.1.4.0                                  P7: 000000000000906c
  P8: c0000005                                 P9: 0000000000000008
  ```

  Rainmeter calls `FreeLibrary` on a plugin as soon as its last measure is gone. 2.1.4 made
  every category poll on its own thread and deliberately leaked `HardwareCore` so that no
  thread is ever joined under the loader lock, which fixed the freeze — but it left those
  threads running with nothing to stop them, so the unload unmapped the code they were
  executing. `_unloaded` in the faulting-module name is that, literally.

  The module is pinned now (`GetModuleHandleEx` with `GET_MODULE_HANDLE_EX_FLAG_PIN`), so
  Rainmeter's unload is a no-op and the plugin's code stays mapped for the life of the
  process. Nothing is lost by that: a DLL a loaded skin has mapped was never replaceable
  anyway. Unregistering the last measure now also stops every poller, so a pinned module
  with no measures left does no work and touches no driver.

  Stopping a poller no longer waits for its thread either, anywhere. The stop event is
  owned jointly by the poller and its thread, so signalling it is safe while a gather is
  still in flight — Rainmeter's thread is never blocked for the length of a driver call,
  and the thread closes the handle as it exits. Verified over repeated
  deactivate/reactivate cycles: no crash, no thread or handle growth, and polling restarts
  with live values after a full stop.

### Changed

- **The skins folder is `NHM` now, not `plugin_test`,** so every skin in the package is a
  variant of one config in Rainmeter's menu instead of a folder named after a debug session.

### Added

- **`NHM\overview.ini`, the skin the package opens on install.** One panel with the hardware
  worth seeing at a glance — CPU usage and clock, RAM, GPU name, load, temperature and VRAM,
  network throughput, drive space — with bars on the three that have a range. The
  per-category skins are still there as variants and still carry every metric the plugin
  exposes; they are reference material, and this is the one meant to stay on a desktop.

## [2.1.4] — 2026-10-10

### Fixed

- **Rainmeter froze permanently whenever the plugin was unloaded.** Refreshing a skin until
  the last NHM measure went away, `!Quit`, or an installer replacing the DLL would leave
  Rainmeter unresponsive with the CPU idle and nothing further in the log. Captured stack of
  a hung instance:

  ```
  Rainmeter -> KERNELBASE!FreeLibrary -> ntdll!LdrUnloadDll -> LdrpProcessDetachNode
    -> NativeHardwareMonitor!dllmain_crt_process_detach
    -> ucrtbase!execute_onexit_table
    -> dynamic atexit destructor for 'g_ADLX' -> ADLXHelper::Terminate
    -> amdadlx64!ADLXTerminate -> atiadlxx!ADL2_FeatureSync_Stop
    -> KERNELBASE!WaitForSingleObjectEx -> ntdll!NtWaitForSingleObject
  ```

  The ADLX SDK's `ADLXHelper` is a file-scope global whose destructor calls
  `ADLXTerminate()`, so it ran from `DllMain(DLL_PROCESS_DETACH)` with the loader lock
  held. `ADLXTerminate` waits on an `amdadlx64` worker thread that cannot finish while that
  lock is held — a deadlock with no timeout. The plugin now uses its own `ADLXHelper` that
  is deliberately never destroyed, leaving the SDK's global uninitialised so its destructor
  returns immediately.

  `HardwareCore`'s singleton was the same hazard and is leaked for the same reason: as a
  static it was destroyed at process detach, where its destructor joined every poller
  thread (a thread cannot run `DLL_THREAD_DETACH` under the loader lock) and called
  `nvmlShutdown`, NVAPI unload, ADL2 destroy and `FreeLibrary`. That also freed modules out
  from under a gather still in flight, which is the `nvml.dll` `c0000005` Windows Error
  Reporting logged ten times in one evening.

- **No vendor driver is called from Rainmeter's thread any more.** `GetValue` used to run
  `Initialize()` and `GatherAll()` inline whenever no skin set `UpdateOverride` — which is
  the default — so provider initialisation and every gather happened on the UI thread while
  holding the module mutex. One slow NVML or ADL call froze the whole application, worst of
  all at skin load, where init and a full gather happen in the same call. Every category now
  polls on its own thread whether or not an override is set, and readers take the lock with
  a 2 ms timeout, returning the value they last reported rather than waiting on hardware.
  `UpdateOverride=` still tightens the cadence; the default is 1 s.

## [2.1.3] — 2026-10-10

### Changed

- **`Debug=2` now dumps every device in the category, not just the measure's own.** One
  measure with `Debug=2` writes `<Category>_dev0.txt` through `<Category>_devN-1.txt` for
  however many devices the category enumerates. Diagnosing a device that is *missing* used
  to be impossible: the dump was written for the measure's own `Device=`, and a measure
  pointing at a device that does not exist is invalid, so it wrote nothing at all — which is
  exactly how the ADL2 enumeration bug fixed in 2.1.2 stayed hidden. The reporter's GPU
  measure could only ever produce `GPU_dev0.txt`, and the absence of `GPU_dev1.txt` read as
  "the dump is broken" rather than "there is no device 1". If the category enumerates
  nothing at all, the requested index is still dumped so the header's `device N of 0` says
  so out loud. Note that a category with many devices now produces one file per device — a
  `Debug=2` CPU measure writes one per core plus the total.
- **The `Debug=2` throttle is now per category, not per measure.** A dump calls `GatherAll()`
  under the module mutex on Rainmeter's own thread, and the test skins set `Debug=2` on every
  measure — `gpu.ini` has 26 of them. Throttled per measure, that became roughly thirteen full
  hardware gathers a second on the UI thread and Rainmeter stopped responding within seconds of
  the skin loading. One dump per category per interval is all a diagnostic needs, since every
  measure in a category reads the same snapshot. Verified with `gpu.ini` loaded: responsive for a
  minute, flat memory, `GPU_dev0.txt` and `GPU_dev1.txt` both written.

## [2.1.2] — 2026-10-10

### Fixed

- **AMD GPUs were never enumerated through ADL2.** ADL reports AMD's vendor id as the
  decimal number `1002` (`0x3EA`), not the PCI id `0x1002`. `Adl2Provider` compared against
  `0x1002`, so every AMD adapter failed the vendor test, the adapter list came back empty,
  and `Initialize()` always reported failure — that provider has not enumerated a single
  device since the 2.0.0 rewrite. It went unnoticed because ADLX is tried first and succeeds
  wherever a reasonably current Adrenalin driver is installed. On a machine without ADLX —
  an older OEM driver that ships no `amdadlx64.dll` — the AMD side therefore registered no
  device at all, which is the integrated-GPU-reads-nothing report from the forum: not a
  metrics bug but an enumeration bug, the dump showing `GPU / device 0 of 1` on a hybrid
  laptop.
- **`ADL2_Adapter_Active_Get` is not usable as an adapter filter.** It returns 0 for every
  adapter, including the discrete GPU currently driving the display (measured on an RTX 4060
  plus a Raphael iGPU: nine adapters reported, none "active"), so it would have dropped
  everything even with the vendor id corrected. Enumeration now gates on `iExist` — physical
  existence — rather than on active or `iPresent`, the latter meaning "has an OS display
  name", which is exactly what an iGPU driving no display lacks.

  Device counts do not change for anyone whose ADLX already worked: a functioning ADL2
  attaches as a per-metric backup on the same device and only fills gaps it finds missing,
  never overriding ADLX or adding a duplicate GPU. It becomes a device in its own right only
  on the ADLX-less path. Metrics on that path come from Overdrive5, which integrated
  Renoir/Cezanne-class GPUs largely do not implement, so such a device may report its name
  and little else.
- **The 32-bit stub and the version resource** go to 2.1.2 together, as always, so the skin
  installer replaces an installed 2.1.1 instead of deciding the versions match.

### Changed

- **The package now opens `plugin_test\gpu.ini` on install** instead of
  `plugin_test\sectionvariables.ini`. Rainmeter was observed to stop responding shortly
  after an install completed, tracking the skin the package opens rather than the install
  itself — `sectionvariables.ini` reads five metrics through both the measure and the
  section-variable path on every tick. The cause is not yet understood; the skin stays in
  the package and can be loaded by hand, it is simply not the one opened automatically.

### Added

- **`tools/adl2_enum_check.py`** — runs the same adapter filter as the C++ against the live
  ADL driver and fails if it enumerates nothing. On the development machine: nine adapters
  reported, one AMD device after the bus deduplication, against zero for the old hex vendor
  comparison and zero for the `Active_Get` gate.

## [2.1.0] — 2026-10-10

### Added

- **Section variables — one measure for every metric.** Each category now exports a
  function callable as a section variable, so a skin no longer needs one measure per
  data point:

  ```ini
  [NHM]
  Measure=Plugin
  Plugin=NativeHardwareMonitor

  [MeterTemp]
  Meter=String
  Text=[NHM:GPU(temp)] / [NHM:GPU(name, 1)] / [NHM:CPU(usage)]
  DynamicVariables=1
  ```

  `GPU`, `CPU`, `Memory`, `Network`, `Storage`, `Ping`, `Battery`, `System`,
  `Motherboard` and `Display`, each taking `(metric, [device], [decimals])`.
  Numbers print with up to three decimals, trailing zeros trimmed, because a
  section variable reaches the meter as text and `NumOfDecimals` can no longer
  act on it; `[NHM:CPU(usage,,0)]` sets the decimals explicitly. Metric names go
  through the same parser as `Metric=`, so every alias works here too, and string
  metrics (`Name`, `DriverVersion`, ...) return their string. Each distinct
  category/metric/device triple registers one handle on first read and is released
  with the measure; the poller still runs once per category however many metrics a
  skin reads. Requested on the forum for Lua, where
  `SKIN:ReplaceVariables('[NHM:GPU("usage")]')` now replaces one
  `SKIN:GetMeasure(...)` plus one measure declaration per value.
- **`Count` metric in every category.** `Metric=Count` or `[NHM:GPU(count)]` returns the
  number of devices, so skins can clamp a device index against the real count instead of
  a hardcoded maximum. Answered centrally from `IModule::GetDeviceCount()`.
- **A measure with no `Category=` is now a valid hub measure.** It has no reading of its
  own (it reads `0`), hosts section variables, and no longer logs a configuration
  warning. A *wrong* `Category=` still warns and still reads `-2`.
- **`package/` + `tools/build-package.ps1`** — the `.rmskin` layout and its build are now
  in the repo instead of being assembled by hand: `RMSKIN.ini`, the `plugin_test` skins,
  and both architecture DLLs, zipped with the footer Rainmeter expects. The script
  refuses to package DLLs whose version resource disagrees with `RMSKIN.ini` or whose
  architecture marker is wrong.
- **`plugin_test/sectionvariables.ini`** — reads five GPU metrics through both paths side
  by side so the columns can be compared, and scrolls between devices by clamping against
  `[NHM:GPU(count)]`. `lua.ini` / `lua_test.lua` now show the single-measure path next to
  the original per-measure one.
- **CI builds `Win32` as well as `x64`**, uploading an artifact per platform.

### Fixed

- **The x64 DLL advertised itself as 32-bit.** `rc.exe` does not define `_WIN64` the way
  `cl.exe` does, so the `.rc`'s `#ifdef _WIN64` always took the 32-bit branch and every
  x64 build — 2.0.0 included — shipped the `(32-bit)` Rainmeter ProductVersion marker.
  The x64 configurations now pass `_WIN64` to `ResourceCompile`, as the Rainmeter SDK's
  own sample plugins do.
- **32-bit plugin had no version resource.** The x86 stub was built outside the project, so
  the `Plugins/32bit/` DLL shipped in 2.0.0 reported an empty FileVersion — the Rainmeter
  skin installer therefore saw it as 0.0.0.0 and could refuse to upgrade it. It is built
  from the project now and carries the same version as the x64 DLL.
- **2.0.0's "Lua support" claim was overstated.** Nothing Lua-specific shipped in 2.0.0;
  plugin measures were readable from Lua because any measure is. The single-call API that
  entry implied is what the section variables above actually add.

### Added (project)

- **`Win32` platform in the project.** Builds `source/Plugin/StubX86.cpp` (now tracked) plus
  the version resource, and nothing else — the real providers stay x64-only. The Win32
  configurations deliberately omit the PostBuildEvent so an x86 build can never overwrite the
  installed 64-bit plugin.

## [2.0.0] — 2026-07-11

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
