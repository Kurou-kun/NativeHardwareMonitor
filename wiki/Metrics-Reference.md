# Metrics Reference

All `Category` and `Metric` values are case-insensitive and whitespace-trimmed (`Category=GPU`, `gpu`, `GpU` are identical).

---

## GPU

`Device=0,1,2...` = which physical GPU (0 = first detected, in enumeration order). Supports NVIDIA (NVML primary, NVAPI backup) and AMD (ADLX primary, ADL2 legacy backup) simultaneously — whichever vendor(s) are actually present get enumerated.

### Numeric metrics

| Metric | Description | NVML | NVAPI | ADLX | ADL2 |
|---|---|:---:|:---:|:---:|:---:|
| `Usage` | GPU utilization % | ✅ | ✅ | ✅ | ✅ |
| `CoreClock` | Core clock, Hz | ✅ | ✅ | ✅ | ✅ |
| `MemoryClock` | Memory clock, Hz | ✅ | ✅ | ✅ | ✅ |
| `Temperature` | Core temp, °C | ✅ | ✅ | ✅ | ✅ |
| `HotspotTemperature` | Hotspot temp, °C | ❌ | ❌ | ✅ | ❌ |
| `MemoryTemperature` | VRAM temp, °C | ✅ | ✅ | ✅ | ❌ |
| `VramUsed` / `VramTotal` | VRAM, bytes | ✅ | ✅ | ✅ | ❌ |
| `FanSpeed` | Fan speed, % | ✅ | ❌ | ✅ | ✅ |
| `FanSpeedRPM` | Fan speed, RPM | ✅ | ✅ | ❌ | ❌ |
| `Power` | Power draw, W | ✅ | ❌ | ✅ | ❌ |
| `PowerLimit` | Power limit, W | ✅ | ❌ | ❌ | ❌ |
| `Voltage` | Core voltage, mV | ❌ | ❌ | ✅ | ❌ |
| `IntakeTemperature` | Intake air temp, °C | ❌ | ❌ | ✅ | ❌ |
| `TotalBoardPower` | Total board power, W | ❌ | ❌ | ✅ | ❌ |

### String metrics

| Metric | Description | NVML | NVAPI | ADLX | ADL2 | Windows fallback |
|---|---|:---:|:---:|:---:|:---:|:---:|
| `Name` | GPU model name | ✅ | ✅ | ✅ | ✅ | ✅ (last resort) |
| `DriverVersion` | Driver version | ✅ (system-wide) | ✅ | ✅ | ❌ | ✅ (last resort) |
| `VbiosVersion` | Video BIOS version | ✅ | ✅ | ✅ | ❌ | ❌ |
| `PciDeviceId` | PCI vendor:device ID, hex | ✅ | ✅ | ✅ | ❌ | ❌ |

A metric missing from the primary provider is automatically filled from the vendor's backup provider (NVML↔NVAPI, ADLX↔ADL2) if that backup can supply it, then finally from a Windows DXGI/registry last resort for `Name`/`DriverVersion` if nothing else can. A blank cell above means it's a genuine hardware/SDK ceiling, not a bug — don't expect it to start working.

---

## CPU

`Device=0` = aggregate/total across all logical cores. `Device=1..N` = individual logical core (`N` = core count).

### Numeric metrics

| Metric | Description | Per-core? |
|---|---|:---:|
| `Usage` | Usage % | ✅ |
| `Clock` | Current clock, Hz | ✅ |
| `MaxClock` | Max clock, Hz (device 0 = peak across cores) | ✅ |
| `Voltage` | Core voltage, mV | ❌ (device 0 only) |

`Voltage` reads `Win32_Processor.CurrentVoltage` via WMI — on most modern boards this returns `-1` because the BIOS reports a legacy placeholder value instead of a real reading (no user-mode API can get real per-core voltage; that needs a kernel driver reading SMU/MSR telemetry, which is out of scope for this plugin — see the project's no-driver policy).

### String metrics

| Metric | Description |
|---|---|
| `Name` | Full CPU name (e.g. `AMD Ryzen 7 9700X 8-Core Processor`) |
| `Vendor` | `AuthenticAMD` / `GenuineIntel` |
| `Identifier` | Architecture string (e.g. `AMD64 Family 26 Model 68 Stepping 0`) |
| `MicrocodeVersion` | Microcode/AGESA revision, hex (e.g. `0xB404023`) |

All four are package-level (same value regardless of `Device` index), sourced from one registry key (`HARDWARE\DESCRIPTION\System\CentralProcessor\0`).

---

## Memory

`Device=0` = RAM, `Device=1` = Virtual, `Device=2` = Swap. Windows conflates "swap" and "virtual memory" more than Linux does — this plugin keeps them distinct:

| Device | What it actually is | Source |
|---|---|---|
| `0` — RAM | Physical memory installed | `GlobalMemoryStatusEx` |
| `1` — Virtual | **Commit Charge** — what Task Manager calls "Committed" (RAM + pagefile combined, i.e. the total your processes can allocate) | `GetPerformanceInfo` |
| `2` — Swap | The literal `pagefile.sys` size/usage on disk | WMI `Win32_PageFileUsage` |

### Metrics (apply to all 3 devices unless noted)

| Metric | Description |
|---|---|
| `Used` / `Free` / `Total` | Bytes |
| `UsedPercent` | % |
| `Cached` | Cached/standby memory, bytes — **Device 0 (RAM) only**, a breakdown of physical memory, not a separate pool |

No string metrics for Memory.

---

## Network

`Device=0,1,2...` = network adapter index (loopback and tunnel adapters are excluded from enumeration).

| Metric | Description |
|---|---|
| `Download` / `Upload` | Current throughput, bytes/sec |
| `DownloadTotal` / `UploadTotal` | Cumulative bytes since adapter init |
| `Speed` | Adapter's negotiated link speed, bits/sec |

*(Expansion of this category — more counters and adapter name/description strings — is planned but not yet released; check back or see the repo's recent commits.)*

No string metrics yet for Network.

---

## Storage

`Device=0,1,2...` = physical disk index.

| Metric | Description |
|---|---|
| `ReadBytes` / `WriteBytes` | Cumulative bytes |
| `ReadSpeed` / `WriteSpeed` | Current throughput, bytes/sec |
| `UsedSpace` / `FreeSpace` / `TotalSpace` | Bytes |

No string metrics yet for Storage.
