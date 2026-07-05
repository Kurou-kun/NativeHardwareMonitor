# Configuring Measures

Every reading is one Rainmeter measure with `Measure=Plugin` and
`Plugin=NativeHardwareMonitor`. What it reads is set by `Category` + `Metric`;
which device by `Device`. All option names and values are **case-insensitive**
and **whitespace-trimmed**.

```ini
[MeasureGpuTemp]
Measure=Plugin
Plugin=NativeHardwareMonitor
Category=GPU
Metric=Temperature
Device=0
```

## Options

### `Category` — *required*

Which subsystem. One of `GPU`, `CPU`, `Memory`, `Network`, `Storage`, `Ping`.
An unknown category makes the measure inert (it reads `-2`) and logs a note.

### `Metric` — *required*

Which reading, within the category. Values are category-dependent — see the full
list in **[Metrics Reference](Metrics-Reference)**. An unknown metric for a valid
category behaves like an unknown category (inert).

### `Device` — default `0`

Which instance. Meaning depends on the category:

| Category | `Device` means |
|---|---|
| GPU | GPU index in enumeration order (`0` = first detected) |
| CPU | `0` = whole-CPU aggregate; `1..N` = individual logical core. Topology/identity metrics live on `0` |
| Memory | `0` = RAM, `1` = Virtual (commit), `2` = Swap (pagefile) |
| Network | Interface index (loopback/tunnel excluded) |
| Storage | Drive-lettered volume, sorted by drive letter |
| Ping | **not used** — use `Host=` instead |

### `Host` — Ping only

Hostname or IP to ping (e.g. `1.1.1.1`, `google.com`). Replaces `Device=` for the
Ping category. Each distinct host gets its own background poller.

### `PingInterval` — Ping only, default `1000`

Milliseconds between ICMP echoes for this host.

### `UpdateOverride` — default `0` (follow Rainmeter)

Milliseconds. When set, this category is polled on a background thread at this rate,
independent of the skin's `Update=`. Omit it to gather in step with Rainmeter's tick.
Useful for a fast network/storage readout under a slow skin, or to throttle an
expensive category.

> **Do not use `UpdateRate`** — that's a reserved Rainmeter keyword. The option is
> named `UpdateOverride` precisely to avoid the collision.

### `Debug` — default `0`

Diagnostic logging to the Rainmeter log (About → Log). Never logs metric *values*.

| Value | Logs |
|---|---|
| `0` | Silent (plus always-on init/error events) |
| `1` | Startup: this measure's registration |
| `2` | All registration/reload events |

The plugin tracks the **max** `Debug` across all measures, so one measure at `2`
raises verbosity globally.

## Reading the value in a meter

Rainmeter measures expose a numeric side and a string side; the plugin fills whichever
the metric is.

- **Numeric** metrics (`Usage`, `Temperature`, clocks, bytes…) → `%1` shows the number.
  Bind these to any meter (String, Bar, Line, Histogram, Roundline…).
- **String** metrics (`Name`, `Vendor`, `VolumeLabel`, `ThrottleReasons`, `MemoryType`…)
  → `%1` shows the text, but **only in a `Meter=String`**. Bound to a numeric-only meter
  (Bar/Line/Histogram) a string metric reads `-1`.

```ini
; numeric — clocks come out in Hz, so autoscale
[MeasureCoreClock]
Measure=Plugin
Plugin=NativeHardwareMonitor
Category=GPU
Metric=CoreClock
[MeterClock]
Meter=String
MeasureName=MeasureCoreClock
AutoScale=2k
Text=Core: %1Hz

; string — must be a String meter
[MeasureGpuName]
Measure=Plugin
Plugin=NativeHardwareMonitor
Category=GPU
Metric=Name
[MeterName]
Meter=String
MeasureName=MeasureGpuName
Text=%1
```

## Units at a glance

- Clocks: **Hz** (converted from MHz — use `AutoScale=2k`).
- Sizes / throughput: **bytes** (or bytes/sec).
- Percentages: `0–100`.
- Network link speed: **bits/sec**.

Full per-metric detail: **[Metrics Reference](Metrics-Reference)**.
For what `-1` / `-2` mean and common surprises: **[Quirks & Troubleshooting](Quirks-and-Troubleshooting)**.
