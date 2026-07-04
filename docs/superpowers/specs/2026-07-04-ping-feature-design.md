# Ping Feature Design

## Goal

Implement the `Measure_Ping` placeholder already present in `plugin_test\network.ini` (`Measure=String, String=0`, tagged `[TO IMPLEMENT]`) as a real ICMP ping measurement: round-trip time and rolling packet loss to a skin-configurable host.

## Why a new Category::Ping instead of a NetworkMetric

Every other category (`GPU`, `CPU`, `Memory`, `Network`, `Storage`) is keyed by physical device index — `Device=0,1,2...` enumerates hardware the OS already exposes (NICs, GPUs, disks). Ping has no such enumeration: the target is an arbitrary string (`Host=8.8.8.8` or `Host=example.com`) supplied per-measure, and a single measure can block for the full ICMP timeout when the target is unreachable. Bolting this onto `NetworkModule` would make `Device=` mean two incompatible things (NIC index vs. ping-target slot) and let a dead ping target stall bandwidth/packet metrics sharing the same `GatherAll()` cycle. A dedicated category keeps target-string identity and independent-failure isolation without overloading the existing device-index model.

## Architecture

```
Plugin.cpp (Host=, PingInterval= options)
  -> HardwareCore::RegisterMeasure(..., targetHost, pingIntervalMs)
       -> IModule::ResolveTarget(host, intervalMs) -> slotIndex   [new virtual, default no-op]
            -> PingModule -> PingResolver -> WinApiPingProvider
                 - normalizes host (trim + lowercase) as dedup key
                 - first sight of a host: GetAddrInfoW resolve, assign next slot,
                   create a PollerThread that pings that target forever
                 - repeat sight of a host: reuse slot; if new intervalMs is smaller,
                   restart that target's PollerThread at the smaller interval
                   (same "min wins" rule HardwareCore::UpdatePoller already uses)
```

`PingModule`/`PingResolver` mirror the thin delegate shape of `NetworkModule`/`NetworkResolver` — no new architectural pattern, just a new instance of the existing one.

`IModule` gains exactly one new method:
```cpp
virtual uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) { return 0; }
```
Every other module inherits the default and is unaffected.

## Threading model

Reuses `Core/PollerThread` (already used for per-category `UpdateOverride` polling) as-is — no new thread-management code. Each unique ping target owns:
- its own `PollerThread`, started the moment the target is first resolved, running for the plugin's lifetime regardless of any skin's `UpdateOverride=`
- its own `IcmpHandle` (`IcmpCreateFile`), so one target's blocking `IcmpSendEcho` call never contends with another's
- a small per-target mutex guarding its cached `{ rtt, packetLoss }` pair and a fixed-size (20-entry) ring buffer of hit/miss results

`WinApiPingProvider::GatherSnapshot(slotIndex, snap)` just copies the cached values under that target's mutex — cheap, matches the existing pull-on-read pattern in `HardwareCore::GetValue`. No special-casing needed there.

## Metrics

New `Types/PingMetric.h`:
```cpp
enum class PingMetric : uint32_t { Rtt, PacketLoss, Unknown };
```
- `Rtt` — milliseconds, from `IcmpSendEcho`'s `RoundTripTime` reply field.
- `PacketLoss` — percentage of misses in the last 20 ping attempts for that target.

`MetricParser.cpp` gets a new `Category::Ping` case (`"rtt"`, `"packetloss"`) and `ParseCategory` gets `"ping"`.

No string metrics — `WinApiPingProvider::GetString` always returns `false` (same stub shape `NetworkMetric` originally had).

## Failure handling

One failure path, not two: DNS resolution failure and ICMP timeout both report `Rtt = 9999` (a fixed sentinel, diverging from this project's usual `-1 = unsupported / -2 = plugin error` convention on purpose — matches the common Rainmeter ping-plugin idiom of a graph-breaking spike rather than a negative value). `PacketLoss` reflects the same miss in its ring buffer regardless of which failure caused it.

Resolution happens once, at target creation. If it fails, the target still gets a slot and its `PollerThread` still runs — each tick reports the 9999 sentinel — rather than crashing or removing the measure. (No retry-storming DNS for a genuinely bad hostname; if the user fixes the host string, a skin `!Refresh` re-registers and re-resolves.)

## Registration/config plumbing

`Plugin.cpp::ReadContext` reads two more options unconditionally (harmless no-ops for non-Ping categories):
- `Host=` (string, `RmReadString`, default `""`)
- `PingInterval=` (int ms, `RmReadInt`, default `1000`)

`HardwareCore::RegisterMeasure` gains two new trailing default params (`targetHost = L""`, `pingIntervalMs = 0`); when `category == Category::Ping` it calls `ResolveTarget` to get the real device index instead of trusting the skin's `Device=` (which is meaningless for Ping and ignored).

## Test skin

`plugin_test\network.ini`'s existing `[Measure_Ping]` stub becomes:
```ini
[Measure_Ping]
Measure=Plugin
Plugin=#plugin_name#
Category=Ping
Metric=rtt
Host=8.8.8.8
Debug=2
```
plus a new `[Measure_PacketLoss]` measure/meter pair (`Metric=packetloss`) next to it. Stays in the existing Network test skin rather than a separate `ping.ini`.

## Explicitly out of scope (YAGNI)

- IPv6 targets (`Icmp6SendEcho2`) — IPv4 only via `GetAddrInfoW`/`IcmpSendEcho`.
- Configurable ring-buffer size for packet loss — fixed at 20.
- Configurable ICMP timeout — fixed at `min(pingIntervalMs, 1000)` so a slow timeout can't outlive its own interval.
- String metrics for Ping (e.g. echoing back the resolved IP or original host string).
- Re-resolving a target's DNS entry after the first successful/failed resolution.
