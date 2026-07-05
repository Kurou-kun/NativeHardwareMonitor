# Metrics Reference

All `Category` and `Metric` values are **case-insensitive and whitespace-trimmed** (`Category=GPU`, `gpu`, ` GpU ` are identical).

Units: clocks are reported in **Hz** (converted from MHz — use `AutoScale=2k` in the meter). Sizes are in **bytes**. Percentages are `0–100`.

Jump to: [GPU](#gpu) · [CPU](#cpu) · [Memory](#memory) · [Network](#network) · [Storage](#storage) · [Ping](#ping)

---

## GPU

`Device=0,1,2…` = which GPU (0 = first detected, enumeration order). NVIDIA (NVML primary, NVAPI backup) and AMD (ADLX primary, ADL2 legacy backup) are supported simultaneously — whichever vendor(s) are present get enumerated. A metric a provider can't supply is filled from the vendor's backup provider, then from a Windows DXGI/registry last resort for `Name`/`DriverVersion`. A blank cell below is a genuine hardware/SDK ceiling, not a bug.

### Numeric metrics

| Metric | Description | NVML | NVAPI | ADLX | ADL2 |
|---|---|:---:|:---:|:---:|:---:|
| `Usage` | GPU utilization % | ✅ | ✅ | ✅ | ✅ |
| `CoreClock` | Core clock, Hz | ✅ | ✅ | ✅ | ✅ |
| `MemoryClock` | Memory clock, Hz | ✅ | ✅ | ✅ | ✅ |
| `MaxCoreClock` | Max core clock, Hz | ✅ | ❌ | ❌ | ❌ |
| `Temperature` | Core temp, °C | ✅ | ✅ | ✅ | ✅ |
| `HotspotTemperature` | Hotspot temp, °C | ❌ | ❌ | ✅ | ❌ |
| `MemoryTemperature` | VRAM temp, °C | ✅ | ✅ | ✅ | ❌ |
| `IntakeTemperature` | Intake air temp, °C | ❌ | ❌ | ✅ | ❌ |
| `VramUsed` / `VramTotal` | VRAM, bytes | ✅ | ✅ | ✅ | ❌ |
| `FanSpeed` | Fan speed, % | ✅ | ❌ | ✅ | ✅ |
| `FanSpeedRPM` | Fan speed, RPM | ✅ | ✅ | ❌ | ❌ |
| `Power` | Power draw, W | ✅ | ❌ | ✅ | ❌ |
| `PowerLimit` | Power limit, W | ✅ | ❌ | ❌ | ❌ |
| `TotalBoardPower` | Total board power, W | ❌ | ❌ | ✅ | ❌ |
| `Voltage` | Core voltage, mV | ❌ | ❌ | ✅ | ❌ |
| `PcieLinkGen` | Current PCIe generation (1–5) | ✅ | ❌ | ✅¹ | ❌ |
| `PcieLinkWidth` | Current PCIe lane count (e.g. 8, 16) | ✅ | ❌ | ✅¹ | ❌ |
| `EncoderUsage` | NVENC video-encoder load, % | ✅ | ❌ | ❌ | ❌ |
| `DecoderUsage` | NVDEC video-decoder load, % | ✅ | ❌ | ❌ | ❌ |
| `PerfState` | Performance state, `0`–`15` (`0` = max) | ✅ | ❌ | ❌ | ❌ |

¹ ADLX PCIe gen/width are implemented but **not yet runtime-tested on AMD hardware** — verify on an AMD GPU.

### String metrics

| Metric | Description | NVML | NVAPI | ADLX | ADL2 | Windows fallback |
|---|---|:---:|:---:|:---:|:---:|:---:|
| `Name` | GPU model name | ✅ | ✅ | ✅ | ✅ | ✅ (last resort) |
| `DriverVersion` | Driver version | ✅ (system-wide) | ✅ | ✅ | ❌ | ✅ (last resort) |
| `VbiosVersion` | Video BIOS version | ✅ | ✅ | ✅ | ❌ | ❌ |
| `PciDeviceId` | PCI `vendor:device` ID, hex (e.g. `10DE:2882`) | ✅ | ✅ | ✅ | ❌ | ❌ |
| `ThrottleReasons` | Why clocks are capped: `None` / `Idle` / `Power Limit` / `Thermal (HW)` / `Thermal (SW)` / `HW Slowdown` / … (comma-joined) | ✅ | ❌ | ❌ | ❌ | ❌ |

---

## CPU

`Device=0` = whole-CPU aggregate. `Device=1..N` = individual logical core (`N` = logical processor count). Topology and identity metrics live on **device 0**.

### Numeric metrics

| Metric | Description | Per-core? |
|---|---|:---:|
| `Usage` | Usage % | ✅ |
| `Clock` | Current clock, Hz | ✅ |
| `MaxClock` | Max clock, Hz (device 0 = peak across cores) | ✅ |
| `Voltage` | Core voltage, mV — usually `-1` (see below) | ❌ (device 0) |
| `CoreCount` | Physical core count | device 0 |
| `ThreadCount` | Logical processor (thread) count | device 0 |
| `CacheL1` / `CacheL2` / `CacheL3` | Total cache per level, bytes | device 0 |
| `Architecture` | *(string)* `x64` / `ARM64` / `ARM` / `x86` | device 0 |

`Voltage` reads `Win32_Processor.CurrentVoltage` via WMI; on most boards this is a legacy placeholder, so it returns `-1`. Real per-core voltage needs SMU/MSR access via a kernel driver — out of scope. See [Quirks](Quirks-and-Troubleshooting#no-cpu-temperature-or-real-voltage-without-a-driver).

> **Note:** `ThreadCount` reflects the actual OS view. With SMT/Hyper-Threading disabled in BIOS, `ThreadCount == CoreCount`.

### String metrics (device-independent — same value on any `Device`)

| Metric | Description |
|---|---|
| `Name` | Full CPU name (e.g. `AMD Ryzen 7 9700X 8-Core Processor`) |
| `Vendor` | `AuthenticAMD` / `GenuineIntel` |
| `Identifier` | Architecture string (e.g. `AMD64 Family 26 Model 68 Stepping 0`) |
| `MicrocodeVersion` | Microcode revision, hex (e.g. `0xB404023`) |

---

## Memory

`Device=0` = RAM, `Device=1` = Virtual, `Device=2` = Swap. Windows conflates "swap" and "virtual memory"; this plugin keeps them distinct:

| Device | What it is | Source |
|---|---|---|
| `0` — RAM | Physical memory installed | `GlobalMemoryStatusEx` |
| `1` — Virtual | **Commit Charge** (Task Manager's "Committed": RAM + pagefile combined) | `GetPerformanceInfo` |
| `2` — Swap | The literal `pagefile.sys` size/usage on disk | WMI `Win32_PageFileUsage` |

### Usage metrics (all three devices unless noted)

| Metric | Description |
|---|---|
| `Used` / `Free` / `Total` | Bytes |
| `UsedPercent` | % |
| `Cached` | Cached/standby memory, bytes — **RAM (device 0) only** |

### RAM hardware identity — **device 0 only**

| Metric | Description |
|---|---|
| `Speed` | Configured (EXPO/XMP) memory speed, MT/s — falls back to rated speed |
| `MemoryType` | *(string)* `DDR4` / `DDR5` / `LPDDR5` / … |
| `Manufacturer` | *(string)* module maker — may read `Unknown` if the SMBIOS field is unpopulated |
| `PartNumber` | *(string)* module part number (e.g. `F5-6000J3636F16G`) |
| `ModuleCount` | Number of installed modules |

Identity is read from the first module (sticks are near-always identical) plus a total count.

---

## Network

`Device=0,1,2…` = network interface index (device-index order). Loopback/tunnel adapters are excluded.

### Numeric metrics

| Metric | Description |
|---|---|
| `Download` / `Upload` | Current throughput, bytes/sec |
| `DownloadTotal` / `UploadTotal` | Cumulative bytes on the interface |
| `Speed` | Transmit link speed, bits/sec |
| `ReceiveLinkSpeed` | Receive link speed, bits/sec |
| `PacketsReceived` / `PacketsSent` | Cumulative packet counts |
| `ErrorsReceived` / `ErrorsSent` | Cumulative error counts |
| `DiscardsReceived` / `DiscardsSent` | Cumulative discard counts |
| `Mtu` | Interface MTU, bytes |

### String metrics

| Metric | Description |
|---|---|
| `Alias` | Friendly interface name (e.g. `Ethernet`, `Wi-Fi`) |
| `Description` | Adapter description (driver/model string) |
| `PhysicalAddress` | MAC address |
| `ConnectionStatus` | `Up` / `Down` (operational status) |

---

## Storage

`Device=0,1,2…` = drive-lettered volume, sorted by drive letter. Volumes without a drive letter are not enumerated.

### Numeric metrics

| Metric | Description |
|---|---|
| `ReadBytes` / `WriteBytes` | Cumulative bytes |
| `ReadSpeed` / `WriteSpeed` | Current throughput, bytes/sec |
| `ReadsPerSec` / `WritesPerSec` | IOPS |
| `QueueLength` | Average disk queue length (unitless) |
| `BusyPercent` | Disk busy %, `100 − idle%` |
| `UsedSpace` / `FreeSpace` / `TotalSpace` | Bytes |

### String metrics

| Metric | Description |
|---|---|
| `VolumeLabel` | Volume label; unlabeled drives fall back to `Local Disk` / `Removable Disk` (Explorer style) |
| `FileSystem` | e.g. `NTFS`, `exFAT` |
| `DriveType` | `Fixed` / `Removable` / `Network` / `CD-ROM` / `RAM Disk` |

Rate metrics (speeds, IOPS, throughput) read from PDH counters and need **two samples** — expect a garbage/zero value on the very first update cycle.

---

## Ping

Ping does **not** use `Device=`. Each measure specifies a `Host=` (hostname or IP); every distinct host becomes its own target with its own background poller. `PingInterval=` (ms, default `1000`) sets how often the ICMP echo fires.

| Metric | Description |
|---|---|
| `Rtt` | Round-trip time, ms. On timeout or unresolved host, reads `9999` |
| `PacketLoss` | Packet loss, % over recent samples |

```ini
[MeasurePing]
Measure=Plugin
Plugin=NativeHardwareMonitor
Category=Ping
Host=1.1.1.1
PingInterval=2000
Metric=Rtt
```

See [Quirks](Quirks-and-Troubleshooting#ping-specifics) for DNS-retry and reachability behavior.
