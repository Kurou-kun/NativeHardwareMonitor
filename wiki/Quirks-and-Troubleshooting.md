# Quirks & Troubleshooting

## Value semantics

Every numeric read means one of three things:

| Value | Meaning |
|---|---|
| `-2` | Plugin-level error — bad handle, or the module/category failed to initialize |
| `-1` | Not supported on this hardware/driver/BIOS — **not an error**, genuinely unavailable |
| `0` or positive | A real reading |

`-1` is common and expected — no vendor SDK or motherboard exposes *every* value. It's
distinct from a real `0`: internally an unsupported metric carries `supported = false`,
never conflated with a zero reading.

## A string metric reads `-1`

String metrics (`Name`, `Vendor`, `VolumeLabel`, `ThrottleReasons`, `MemoryType`,
`Architecture`, …) only fill the measure's **string** side. Bind one to a numeric-only
meter (Bar, Line, Histogram, Roundline) and you get `-1`, because those meters read the
number. Put string metrics in a `Meter=String` and reference `%1`. See
[Configuring Measures](Configuring-Measures#reading-the-value-in-a-meter).

## No CPU temperature or real voltage without a driver

There is no CPU temperature metric, and CPU `Voltage` usually reads `-1`.

Real per-core voltage and true die temperature (Tctl/Tdie) are only readable through
Model-Specific Registers (MSR) or vendor SMU telemetry — both need a signed **kernel
driver** plus a service to load it. This plugin ships a single user-mode DLL and installs
nothing else, permanently, by design. So these values aren't "coming later"; they're out
of scope.

`Voltage` does try `Win32_Processor.CurrentVoltage` over WMI, but on most boards that
field is a legacy placeholder, so it returns `-1` even where a number might appear in other
tools (those tools use a driver). This is a BIOS/WMI limitation a driverless fix can't
solve anyway.

## First reading is garbage/zero (rate metrics)

Storage speeds/IOPS/throughput and other PDH-counter-based rates need **two samples** to
compute a delta. On the very first update cycle after a measure loads, expect a zero or
junk value — it settles on the second tick.

## Clocks look tiny — they're in Hz

All clocks report in **Hz**, converted from the SDK's MHz. A "5000" MHz core clock reads
`5000000000`. Add `AutoScale=2k` to the meter (or divide) to display GHz/MHz.

## RAM identity may say `Unknown`

`Manufacturer` and similar RAM-identity strings come from SMBIOS/WMI. If the motherboard
leaves an SMBIOS field unpopulated, the value is `Unknown` — a firmware gap, not a plugin
bug. Identity is read from the first module (sticks are near-always identical) plus a
total `ModuleCount`.

## AMD PCIe gen/width unverified

`PcieLinkGen` and `PcieLinkWidth` on the AMD/ADLX path are implemented but not yet
runtime-tested on AMD hardware. Treat them as provisional until confirmed on an AMD GPU;
the NVIDIA/NVML path for the same metrics is verified.

## Ping specifics

- `Rtt` is milliseconds. On a timeout **or** an unresolved host it reads `9999` — a
  sentinel, not a real 9.999 s round-trip.
- `PacketLoss` is a percentage over recent samples.
- Ping ignores `Device=`. Each measure names a `Host=`; every distinct host becomes its own
  target with its own background poller, firing every `PingInterval` ms (default `1000`).
- An unresolvable host keeps being retried on the interval, so a name that resolves later
  starts reporting without a reload.

## Nothing shows up / measure is inert

- Confirm the DLL is at `%APPDATA%\Rainmeter\Plugins\NativeHardwareMonitor.dll` and you
  restarted Rainmeter after copying it.
- A `-2` means the category or metric name didn't parse. They're case-insensitive but must
  match a known value — check spelling against [Metrics Reference](Metrics-Reference).
- Set `Debug=1` on the measure and read the Rainmeter log (About → Log) to see whether it
  registered. It never logs metric values, only registration/init events.
