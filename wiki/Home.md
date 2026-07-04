# NativeHardwareMonitor

A native C++ Rainmeter plugin that reads real-time hardware metrics (GPU, CPU, Memory, Network, Storage) directly from vendor APIs (NVML, NVAPI, ADLX, ADL2) and Windows APIs — no third-party monitoring tool required.

## Requirements

- Windows 10/11, x64
- [Rainmeter](https://www.rainmeter.net/) installed
- NVIDIA GPU: NVML/NVAPI ship with the driver, nothing extra needed
- AMD GPU: ADLX/ADL2 ship with the driver, nothing extra needed
- No kernel driver, no service, no elevated install — the plugin is a single DLL

## Installation

1. Download `NativeHardwareMonitor.dll` (or build it yourself — see below)
2. Copy it into `%APPDATA%\Rainmeter\Plugins\`
3. Restart Rainmeter
4. Reference it from any skin measure with `Plugin=NativeHardwareMonitor`

### Building from source

Open `NativeHardwareMonitor.slnx` in Visual Studio (2022+), build `Release|x64`. The post-build step copies the DLL into `%APPDATA%\Rainmeter\Plugins\` automatically (skip this step on CI runners — it's guarded with an `if exist` check). Rainmeter must be closed while building, since it locks the DLL while loaded.

## Basic usage

```ini
[MeasureExample]
Measure=Plugin
Plugin=NativeHardwareMonitor
Category=GPU
Metric=Temperature
Device=0
Debug=0
```

| Option | Required | Values | Notes |
|---|---|---|---|
| `Category` | yes | `GPU`, `CPU`, `Memory`, `Network`, `Storage` | Case-insensitive |
| `Metric` | yes | category-dependent — see [Metrics Reference](Metrics-Reference) | Case-insensitive |
| `Device` | no (default `0`) | integer index | Which physical device/core/adapter/disk — see per-category notes |
| `Debug` | no (default `0`) | `0`, `1`, `2` | `1` = startup-only logging, `2` = all registration/reload events. Never logs the actual metric *values*. |
| `UpdateOverride` | no | milliseconds | Poll this category faster than the skin's own `Update=` rate. Omit to just follow Rainmeter's normal tick. (`UpdateRate` is a reserved Rainmeter keyword — don't use it here.) |

## Reading the value

Every measure exposes **both** a numeric value and (for some metrics) a string value — Rainmeter picks whichever fits the meter:

```ini
[MeterExample]
Meter=String
MeasureName=MeasureExample
Text=%1
```

- Numeric-only metrics (e.g. `Usage`, `Temperature`) — `%1` shows the formatted number.
- String-typed metrics (e.g. GPU `Name`, CPU `Vendor`) — `%1` shows the actual text, but **only inside a `Meter=String`**. Binding a string metric to a Bar/Histogram/Line meter shows `-1` instead, since those meter types only ever read the numeric side.

### Value semantics (numeric side)

| Value | Meaning |
|---|---|
| `-2` | Plugin-level error (bad handle, module failed to initialize) |
| `-1` | This metric isn't supported on your hardware/driver/BIOS — not an error, just genuinely unavailable here |
| `0` or positive | A real reading |

`-1` shows up often and is expected — not every vendor SDK or every motherboard exposes every value (e.g. AMD GPUs don't expose `FanSpeedRPM`; most BIOSes don't expose real CPU voltage at all). See [Metrics Reference](Metrics-Reference) for the per-category support matrix.

## Full metric list, per category

See **[Metrics Reference](Metrics-Reference)** for every `Category`/`Metric` combination, which vendor/API each comes from, and known hardware limitations.
