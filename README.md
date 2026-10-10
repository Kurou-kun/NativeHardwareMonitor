> ⚠️ **AI-generated project.** The code, documentation, and wiki in this
> repository were written largely by an AI agent. It works on the author's
> hardware, but it has not been independently audited. Read it before you run
> it, and use it at your own risk.

# NativeHardwareMonitor

A native C++ [Rainmeter](https://www.rainmeter.net/) plugin that reads real-time
hardware metrics — **GPU, CPU, Memory, Network, Storage, and Ping** — directly
from vendor APIs (NVML, NVAPI, ADLX, ADL2) and Windows APIs (WMI, PDH, Win32).
No third-party monitoring tool, no kernel driver, no service — a single DLL.

## Requirements

- Windows 10/11, **x64** (Rainmeter and this plugin are both 64-bit)
- [Rainmeter](https://www.rainmeter.net/) installed
- NVIDIA GPU: NVML/NVAPI ship with the driver — nothing extra
- AMD GPU: ADLX/ADL2 ship with the driver — nothing extra
- No kernel driver, no service, no elevated install

## Install

1. Get `NativeHardwareMonitor.dll` (download a release, or build — see the wiki)
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

[MeasureCpuLoad]
Measure=Plugin
Plugin=NativeHardwareMonitor
Category=CPU
Metric=Usage
```

A value of `-1` means the metric is unsupported by your hardware/SDK; `-2` means
a plugin error. Everything else is a real reading.

## One measure, every metric

A measure with no `Category=` is a hub: read any metric off it as a section
variable, instead of declaring a measure per value.

```ini
[NHM]
Measure=Plugin
Plugin=NativeHardwareMonitor

[MeterStats]
Meter=String
Text=[NHM:GPU(name)] — [NHM:GPU(temp)]C — [NHM:CPU(usage)]% — [NHM:GPU(count)] GPUs
DynamicVariables=1
```

One function per category — `GPU`, `CPU`, `Memory`, `Network`, `Storage`,
`Ping`, `Battery`, `System`, `Motherboard`, `Display` — each taking
`(metric, [device], [decimals])`. The metric name accepts the same aliases as
`Metric=`. Options need `DynamicVariables=1` to re-resolve; actions do not.

Numbers print with up to three decimals, trailing zeros trimmed, so whole
numbers and byte counts stay whole. `NumOfDecimals` cannot change that — a
section variable is already text by the time the meter sees it — so pass the
decimals instead: `[NHM:CPU(usage,,0)]` rounds to a whole number (an omitted
middle argument still means device 0).

The same thing from Lua, with no measure per value:

```lua
local usage = tonumber(SKIN:ReplaceVariables('[NHM:GPU("usage")]'))
local name  = SKIN:ReplaceVariables('[NHM:GPU("name")]')
```

`package/Skins/plugin_test/sectionvariables.ini` reads every metric through both
paths side by side, and `lua.ini` does the same from a script.

## Documentation

Full docs live in the **[Wiki](https://github.com/Kurou-kun/NativeHardwareMonitor/wiki)**:

- **[Installation](https://github.com/Kurou-kun/NativeHardwareMonitor/wiki/Installation)** — download/build, deploy the DLL, requirements
- **[Configuring Measures](https://github.com/Kurou-kun/NativeHardwareMonitor/wiki/Configuring-Measures)** — every measure option in detail
- **[Metrics Reference](https://github.com/Kurou-kun/NativeHardwareMonitor/wiki/Metrics-Reference)** — every `Category`/`Metric`, its unit, device indexing, per-vendor support
- **[How It Works](https://github.com/Kurou-kun/NativeHardwareMonitor/wiki/How-It-Works)** — architecture, the update/polling model, GPU fallback
- **[Quirks & Troubleshooting](https://github.com/Kurou-kun/NativeHardwareMonitor/wiki/Quirks-and-Troubleshooting)** — value semantics, what needs a driver, gotchas

## Building

Open `NativeHardwareMonitor.slnx` in Visual Studio 2022 and build the `x64`
configuration, or build from the command line with MSBuild. See the
[Installation](https://github.com/Kurou-kun/NativeHardwareMonitor/wiki/Installation)
page for details.

## Why no kernel driver

CPU temperature and true per-core voltage require SMU/MSR access through a kernel
driver. This project deliberately ships **no driver and no service** — those
metrics are out of scope, and anything they'd expose returns `-1`. Everything
here is readable from user space.

## License

**GNU General Public License v2 or later** — see [`LICENSE`](LICENSE).

This is not a free choice: the Rainmeter plugin SDK header this plugin includes
(`RainmeterAPI.h`) is GPL v2-or-later, and Rainmeter itself is GPLv2. A plugin
that includes that header and links Rainmeter is a derivative work, so it must be
distributed under the GPL. Any fork or derivative of this plugin inherits the same
requirement.
