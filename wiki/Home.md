# NativeHardwareMonitor

A native C++ [Rainmeter](https://www.rainmeter.net/) plugin that reads real-time hardware metrics — **GPU, CPU, Memory, Network, Storage, and Ping** — directly from vendor APIs (NVML, NVAPI, ADLX, ADL2) and Windows APIs (WMI, PDH, Win32). No third-party monitoring tool, no kernel driver, no service — a single DLL.

## Contents

- **[Installation](Installation)** — download/build, deploy the DLL, requirements.
- **[Configuring Measures](Configuring-Measures)** — every measure option in detail.
- **[Metrics Reference](Metrics-Reference)** — every `Category`/`Metric`, its unit, device indexing, and per-vendor support.
- **[How It Works](How-It-Works)** — architecture, the update/polling model, per-vendor GPU fallback.
- **[Quirks & Troubleshooting](Quirks-and-Troubleshooting)** — value semantics (`-1`/`-2`), what needs a driver, and the gotchas.

## Requirements

- Windows 10/11, **x64** (the plugin is 64-bit; Rainmeter is 64-bit)
- [Rainmeter](https://www.rainmeter.net/) installed
- NVIDIA GPU: NVML/NVAPI ship with the driver — nothing extra
- AMD GPU: ADLX/ADL2 ship with the driver — nothing extra
- No kernel driver, no service, no elevated install

## Install

1. Get `NativeHardwareMonitor.dll` (download, or build — see [Installation](Installation))
2. Copy it into `%APPDATA%\Rainmeter\Plugins\`
3. Restart Rainmeter
4. Reference it from any measure with `Plugin=NativeHardwareMonitor`

## Quick example

```ini
[MeasureGpuTemp]
Measure=Plugin
Plugin=NativeHardwareMonitor
Category=GPU
Metric=Temperature
Device=0

[MeterGpuTemp]
Meter=String
MeasureName=MeasureGpuTemp
Text=GPU: %1 °C
```

## Measure options (summary)

| Option | Required | Values | Notes |
|---|---|---|---|
| `Category` | yes | `GPU`, `CPU`, `Memory`, `Network`, `Storage`, `Ping` | Case-insensitive |
| `Metric` | yes | category-dependent — see [Metrics Reference](Metrics-Reference) | Case-insensitive |
| `Device` | no (default `0`) | integer index | Which GPU / core / RAM pool / adapter / disk — meaning is per-category |
| `Host` | Ping only | hostname or IP | The target to ping. Replaces `Device=` for the Ping category |
| `PingInterval` | Ping only (default `1000`) | milliseconds | How often the background ICMP echo fires |
| `UpdateOverride` | no | milliseconds | Poll this category on a background thread at this rate, independent of the skin's `Update=`. Omit to follow Rainmeter's tick. (`UpdateRate` is a reserved Rainmeter keyword — do **not** use it) |
| `Debug` | no (default `0`) | `0`, `1`, `2` | `1` = startup logging, `2` = all registration/reload events to the Rainmeter log. Never logs metric *values* |

Full detail: **[Configuring Measures](Configuring-Measures)**.

## Reading the value

Every measure exposes a numeric value and, for identity metrics, a string value — Rainmeter picks whichever fits the meter:

- **Numeric** metrics (`Usage`, `Temperature`, …) → `%1` shows the number.
- **String** metrics (GPU `Name`, CPU `Vendor`, Storage `VolumeLabel`, GPU `ThrottleReasons`, …) → `%1` shows the text, but **only inside a `Meter=String`**. Bound to a Bar/Line/Histogram meter, a string metric reads `-1` (those meters only use the numeric side).

### Value semantics (numeric side)

| Value | Meaning |
|---|---|
| `-2` | Plugin-level error (bad handle, module failed to initialize) |
| `-1` | Metric not supported on this hardware/driver/BIOS — not an error, genuinely unavailable |
| `0` or positive | A real reading |

`-1` is common and expected — not every vendor SDK or motherboard exposes every value. See [Metrics Reference](Metrics-Reference) for the per-category support, and [Quirks & Troubleshooting](Quirks-and-Troubleshooting) for what's permanently unavailable and why.
