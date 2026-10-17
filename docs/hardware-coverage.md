# What the development machine cannot test

Three bugs shipped in 2026 because a code path was never executed here and failed
silently when it finally ran somewhere else:

| | what broke | where it hid |
|---|---|---|
| [#12](https://github.com/Kurou-kun/NativeHardwareMonitor/issues/12) | Windows GPU counters matched adapter LUIDs case-sensitively | both local GPUs have a working vendor SDK, so the fallback was never the path that answered |
| [#15](https://github.com/Kurou-kun/NativeHardwareMonitor/issues/15) | a BOM in `RMSKIN.ini` made every package uninstallable on Rainmeter 4.5 | this machine runs Rainmeter 5, which tolerates it |
| [#13](https://github.com/Kurou-kun/NativeHardwareMonitor/issues/13) | IGCL published clocks in MHz where every other provider publishes Hz | IGCL has never initialised here at all |

None of them were subtle once seen. All three were invisible because the test
hardware answers through a different path. This page is the inventory, so the next
gap is a known one rather than a surprise.

## The machine

| | |
|---|---|
| GPU 0 | NVIDIA GeForce RTX 4060 — **NVML** primary, **NVAPI** backup, both present |
| GPU 1 | AMD Radeon Raphael iGPU (`1002:13C0`) — **ADLX** primary, **ADL2** backup, both present |
| CPU | AMD Ryzen 7 9700X, 8 cores / 8 threads |
| Storage | 3 disks; one NVMe answers `Temperature`, one is a letterless Linux disk |
| Displays | 4 |
| Network | ~20 adapters, plus Ping |
| Battery | **none** — desktop |
| Rainmeter | 5.0.0.5293, plus a portable 4.5.28 on the Desktop since 2026-10-17 |

## Never executed here

**`IgclProvider`, entirely.** No Intel GPU, so `ControlLib.dll` is absent and the
provider returns at the first line of `Initialize()`. Every reading, unit conversion
and handle in that file is unverified on silicon —
[#2](https://github.com/Kurou-kun/NativeHardwareMonitor/issues/2) exists for this, and
the three MHz-vs-Hz bugs found in the 2026-10-17 revision all lived here. Treat any
change to that provider as unproven until someone runs a dump on an Arc or an Intel
iGPU.

**ADL2 as the *primary* AMD provider.** ADLX initialises here, so ADL2 is only ever a
per-metric backup. The ADL2-alone branch of `GpuResolver` is the one Yincognito's
2020-driver laptop takes, and it is where his `VramUsed` comes from — a value this
machine cannot compare against anything.

**NVML without NVAPI.** NVAPI is always present alongside NVML on a desktop GeForce, so
the branch where NVML's clock stands alone — and the dump says so rather than implying
it — never runs.

**The Windows GPU counters as the only source.** They are attached behind two working
vendor SDKs, so they fill in `VramShared` and nothing else. A GPU with no vendor SDK at
all, which is exactly what that path exists for, is untested.

**Every battery metric's value path.** With no battery, `WinApiBatteryProvider`
initialises and then reports `-1` for all sixteen metrics. The code that formats a real
charge level, capacity or wear figure has never produced a number here.

**A second AMD GPU.** Until 2026-10-17 `GpuResolver` paired ADLX with ADL2 by index,
which is correct for one AMD device and can hand the wrong card's backup readings to a
device on a machine with two. It now pairs on `PciDeviceId` — but the matching code
itself has only ever run against a single AMD GPU.

## Covered, but by one sample only

- **AMD discrete graphics** — one RX 6600 dump from a tester (2026-10-17), which is the
  only evidence ADLX reports a real PCIe link, and the only discrete AMD card to report
  `HotspotTemperature`.
- **A machine with no NVIDIA GPU.** Same dump; everything here always has NVML present.
- **Rainmeter 4.5.** The portable copy proves a package installs; no skin has been run
  under it.
- **Drive temperature.** One NVMe answers, two do not. The "answers" path has a single
  example.

## What this means for a change

A change to `IgclProvider`, to the ADL2-alone branch, to the battery value path or to
the Windows-counters-as-only-source path **cannot be verified here**, however obviously
correct it looks. For those, the discipline that actually helps is the one the revision
settled on: make the failure visible rather than silent. An IGCL telemetry item whose
declared units are not understood now publishes nothing and logs what it saw, so a
reporter's dump says what went wrong instead of carrying a plausible wrong number.

Pure logic extracted out of those paths can still be pinned without hardware — see
`tools/unit_parse_check.cpp`, which covers the PNP-string parsing and the IGCL unit
conversions, and which was confirmed to fail against the pre-fix code before being
trusted.
