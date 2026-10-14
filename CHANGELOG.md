# Changelog

All notable changes to NativeHardwareMonitor are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/);
this project uses a four-part `MAJOR.MINOR.PATCH.BUILD` version in the DLL resource.

## [2.2.9] — 2026-10-14

### Changed

- **NVIDIA clocks now come from NVAPI because of where they come from, not because of how
  big they are.** The 100 MHz floor added in 2.2.3 is gone. It decided between two readings
  by looking at the *value*, and a value cannot tell you whether it is valid — which is
  exactly what it got wrong on the reporter's GTX 1650: NVML's averaged reading of
  **150 MHz** was above the floor, so it was accepted as `CoreClock` while every other tool
  on that machine showed **375 MHz** (forum p=246235). A threshold cannot distinguish a real
  low clock from an averaged one, because both are just numbers.

  The rule is now about provenance: where NVAPI answers, NVAPI *owns* `CoreClock` and
  `MemoryClock` — it is the API MSI Afterburner, GPU-Z and HWiNFO all read, so agreeing with
  it is agreeing with the machine's other tools. NVML's graphics clock keeps being published
  as **`EffectiveClock`**, which is what it is. Where NVAPI is absent, NVML stands as the
  only reading there is, and the dump says so rather than implying it.

  Credit where it is due: Yincognito argued the floor was unsound in principle before his
  own data showed it failing in practice.

### Added

- **A `Debug=2` dump now names the provider behind every value**, not just the chain in its
  header:

      CoreClock           = 3e+08  [NVAPI]
      EffectiveClock      = 9e+06  [NVML]
      VramUsed            = 2.76e+08  [Windows]

  Two APIs can answer for one metric and disagree — that is the whole of the clock story
  above — and until now a dump could not say which one spoke, so reading one meant inferring
  the provider from the number, which is the same mistake the floor made. Each reading now
  carries the name of the provider that produced it, merged values included.

- **`device=` in the `gpu.ini`, `cpu.ini` and `network.ini` reference skins** (and `adapter=`
  for the latter), so looking at a second GPU, a different core or another adapter is one
  edit and a refresh instead of hand-editing every measure in the file. Requested by
  Yincognito, who noticed `gpu.ini` was the one skin that made you do it 26 times.

## [2.2.8] — 2026-10-14

### Changed

- **Section variables stay bare numbers, and that is now a stated contract rather than an
  accident.** A scale argument was built and removed the same day: it would have put unit
  and locale decisions in the plugin, and a skin that reaches for section variables is
  already composing its own text, in Lua or in a `Calc`, where one division is cheaper than
  a formatting dialect to learn.

      local gb = tonumber(SKIN:ReplaceVariables('[NHM:GPU("vramused",,15)]')) / 1073741824

  The printing itself is unchanged - up to three decimals with trailing zeros trimmed, `,,0`
  to round, `,,15` when every digit matters - but it now lives in `source/Utils/ValueFormat.h`
  with the reasoning attached, pinned by `tools/section_format_check.cpp` (18 assertions, no
  framework, compiles standalone).

### Fixed

- Nothing in this plugin was broken by
  **[#10](https://github.com/Kurou-kun/NativeHardwareMonitor/issues/10)**, and the research
  it asked for has a flat answer worth writing down: **Rainmeter's formatting cannot reach a
  plugin section variable, and no syntax can make it.** In `ConfigParser::GetSectionVariable`,
  text after the colon that names a plugin function goes straight to
  `MeasurePlugin::CommandWithReturn` and the returned string *is* the substitution; `:%`,
  `:/1024`, `:MaxValue` and `,decimals` are parsed only on the branch that reads a measure's
  value. So `[NHM:GPU(vramused):/1048576]` is not valid, and `AutoScale` / `NumOfDecimals` on
  the meter act on a measure, not on text that was substituted before the meter saw it.

  The way back into Rainmeter's own formatting is a one-line `Calc` that re-parses the
  number, after which `AutoScale`, `Percentual` and bar ranges all work again — verified on
  live readings and documented in the
  [wiki](https://github.com/Kurou-kun/NativeHardwareMonitor/wiki/Section-Variables#rounding-and-scaling).

## [2.2.7] — 2026-10-12

### Added

- **`Metric=ProviderStatus`** (alias `provider`) — works in every category and answers the
  question `-1` on an ordinary metric cannot: whether this category has a working provider
  at all. `1` initialised and gathering, `0` not up yet (nothing has triggered
  initialisation, or the first gather is still in flight), `-1` no provider on this machine
  — the vendor SDK is absent or initialisation failed.

  So a skin can grey a panel out and say *"no Intel driver"* instead of displaying a
  convincing `-1`, and a bug report can start with a fact. Read without the module lock,
  from state the poller publishes, for the same reason the device count is: a metric whose
  job is to disambiguate `-1` must never itself return a stale `-1`.

  **Not named `Status`** — Battery and Ping already have a string metric by that name and
  hijacking it would break every skin reading them. `tools/metric_parse_check.cpp` asserts
  both: `ProviderStatus` reserved in all ten categories, `status` still resolving to
  `BatteryMetric::Status` and `PingMetric::Status`.

  Narrower than [#9](https://github.com/Kurou-kun/NativeHardwareMonitor/issues/9) asked for
  in one respect: it has no "initialised but the last gather failed" state, because a
  gather cannot report failure — providers do not signal it, they publish `-1` for the
  metrics they could not read. `0` therefore means "not up yet", which is a state that
  genuinely exists and the one a skin meets at load.

## [2.2.6] — 2026-10-12 (shipped in 2.2.7)

### Changed

- **One `Module` class now serves all ten categories, where there were twenty classes.**
  Each category carried a `Module` and a `Resolver` of its own, and nine of the ten pairs
  diffed to **zero lines** against each other once the type names were normalised — they
  differed only in which provider they named and what noun their startup log counted in.
  `GpuResolver` is the one with real work to do (five providers, a per-metric merge, Windows
  counters behind all of them); it is an `IProvider` itself now, so it drops into the shared
  `Module` like any single-source provider. **38 files and ~1180 lines deleted**, no
  behaviour change intended.

  Two visible consequences, both small. A startup log line reads
  `Storage: WinApiStorageProvider initialized (3 drive(s))` where it used to say
  `StorageResolver:` — the resolver it named no longer exists. And the single-device
  categories (Memory, Battery, System, Motherboard) now honour device filtering like the
  rest instead of inheriting "gather everything", which for one device is the same work.

## [2.2.5] — 2026-10-12

### Changed

- **Every category names its metrics in a `Debug=2` dump**, not just GPU and Storage. The ten
  if-chains in `ParseMetric` and the two switches in `MetricName` spelled the same strings
  twice; both are now one table per category, one row per metric, with the canonical name
  derived from the enum member itself so the two cannot drift apart. `Metric=` and section
  variables are unchanged — every name and alias still resolves to the same id, pinned by
  **399 assertions** in the new `tools/metric_parse_check.cpp`.

### Removed

- Dead weight found in a pass over the whole tree, no behaviour change: `g_DebugLevel` (a
  global two sites raised and nothing ever read), `ModuleState::minIntervalMs` (fixed at 100,
  so the cadence it fed was always 1000), `lastGatherTime` and `GetTimeMs()` (written once,
  never read), and the second copy of the PDH counter-array reader — the copies had already
  drifted, one of them missing a `count` reset. `PollerThread`'s callback now captures the
  module state alone instead of four pointers that all hang off it.

## [2.2.4] — 2026-10-12 (shipped in 2.2.5)

### Changed

- **A `Debug=2` Storage dump names its metrics** instead of printing `Metric[11] = 43`. The
  category that most needs it: a drive answering `Temperature` can be a letterless disk whose
  every string metric reads `-1`, leaving a numeric index as the only clue what the reading
  belongs to. GPU and Storage are spelled out now; the other categories still fall back to
  `Metric[<id>]`.

## [2.2.3] — 2026-10-12 (shipped in 2.2.5)

### Fixed

- **A power-gated NVIDIA dGPU reported its effective clock, not its real one.** NVML on an
  Optimus GTX 1650 answered **9 MHz core / 25 MHz memory** where MSI Afterburner, GPU-Z and
  HWiNFO all showed **300 / 405** at the same moment — and `nvidia-smi` agreed with NVML, so
  the conversion was never wrong: NVML was handing back something within a few MHz of
  HWiNFO's *GPU Effective Clock*. Reported and then confirmed by Yincognito
  ([p=246138](https://forum.rainmeter.net/viewtopic.php?p=246138)).

  An NVML clock below 100 MHz no longer sets `CoreClock`, so NVAPI's reading fills it
  through the existing backup merge — the same number every other tool shows. Where NVAPI
  is unavailable such a clock reads `-1` rather than a figure no other tool agrees with.

### Added

- **`Metric=EffectiveClock`** — nothing is discarded by the fix above: NVML's graphics-clock
  reading is published under its own name, the sensor HWiNFO calls *GPU Effective Clock*. On
  a card that is awake it equals `CoreClock`; on a power-gated one it is the far lower
  averaged figure, which is a real measurement and occasionally the more interesting one.
  NVIDIA only — it reads `-1` elsewhere.

## [2.2.2] — 2026-10-12 (shipped in 2.2.5)

### Fixed

- **Drive temperature read `-1` (unsupported) on every disk.** The device path was built
  with one backslash too few — `\.\PhysicalDrive0` is not a device path, so
  `CreateFileW` failed for every drive and each one was permanently marked unsupported on
  its first tick. `tools/storage_temp_probe.py` had the spelling right all along, which is
  why the probe found working sensors while the plugin found none.

## [2.2.1] — 2026-10-11 (shipped in 2.2.5)

### Changed

- **Only the devices a skin reads are gathered now.** Every module read *every* device
  each tick regardless of what was being displayed: this machine enumerates **37 network
  adapters** and **13 CPU devices**, and a skin showing one adapter paid for all 37, every
  second. `HardwareCore` keeps the set of device indices some measure actually reads —
  rebuilt when measures register, not per tick — and the poller gathers only those.

  GPU, CPU, Network and Storage filter; the single-device modules inherit a default that
  gathers everything, which is correct and already cheap.

  **A `Debug=2` dump still sees every device.** A dump exists precisely to show hardware no
  measure reads — that is how the AMD iGPU enumeration bug was found — so the first dump
  switches its category back to gathering the lot. It is not gathered inside the dump
  itself: doing that on Rainmeter's thread under the module mutex is what wedged the app
  before 2.1.3.

## [2.2.0] — 2026-10-11 (shipped in 2.2.5)

### Added

- **`Category=Storage` / `Metric=Temperature`** — drive temperature, read through
  `IOCTL_STORAGE_QUERY_PROPERTY`. This is the only temperature the plugin can read without
  a kernel driver, and it needs **no administrator rights**: the handle is opened with no
  access rights at all, which a property query is enough for.

  **Support is per drive, not per bus.** Of two NVMe drives in the development machine, one
  answers and the other returns `ERROR_IO_DEVICE`. A drive that does not answer reports
  `-1`, is never asked again, and its handle is released.

- **Disks with no mounted volume are enumerated now.** A PDH `PhysicalDisk` instance is
  `"1 C:"` — volume `C:` on `PhysicalDrive1` — or bare `"0"` for a disk carrying no letter.
  Only the lettered ones were listed before, which mattered once temperature arrived:
  **the one drive reporting a temperature on the development machine has no letter.**

  Lettered volumes keep their order and therefore their existing `Device=` indices;
  letterless disks are appended after them, so no existing skin changes meaning. They
  report speeds, queue and temperature; space and label stay `-1`, there being no volume.

  `Device=` still means what it always did. Temperature belongs to the physical disk the
  volume sits on, which is why the disk number is parsed out of the PDH instance name.

### Changed

- **A `Debug=2` dump now carries the provider chain with it.** Provider lines are written
  to the Rainmeter log exactly once, when each provider first initialises — so by the time
  anyone thinks to attach a log, they have scrolled away. A reporter concluded PMLog was
  unavailable on his driver for precisely that reason, when it had in fact initialised and
  was feeding ten metrics. Every dump now opens with what actually answered:

  ```
  --- providers ---
    NvmlProvider: initialized (1 device(s))
    GpuResolver: NVML initialized (1 device(s)), NVAPI available as metric backup
    Adl2Provider: optional calls - PMLog=yes VRAMUsage=yes MemoryInfo2=yes VideoBios=yes
    GpuResolver: Windows GPU counters attached to 2 of 2 device(s)
  ```

  One file now answers the first question every hardware report raises, without asking the
  reporter for anything else.

## [2.1.8] — 2026-10-11

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

## [2.1.7] — 2026-10-11 (shipped in 2.1.8)

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
