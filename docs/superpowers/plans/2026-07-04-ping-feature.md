# Ping Feature Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the long-standing `Measure_Ping` placeholder in `plugin_test\network.ini` as a real ICMP ping measurement (round-trip time + rolling packet loss) to a skin-configurable host, per `docs/superpowers/specs/2026-07-04-ping-feature-design.md`.

**Architecture:** A new `Category::Ping` module (`PingModule` → `PingResolver` → `WinApiPingProvider`) mirrors the existing GPU/CPU/Memory/Network/Storage shape, but "devices" are ping targets keyed by a `Host=` string instead of physical hardware. Each unique host gets its own always-on `PollerThread` (reused as-is from `Core/PollerThread`) doing blocking `IcmpSendEcho` calls, so one dead target never blocks another or the rest of the plugin.

**Tech Stack:** C++17, Win32 IP Helper API (`IcmpSendEcho`/`IcmpCreateFile`, `iphlpapi.lib`), Winsock (`GetAddrInfoW`, `ws2_32.lib`), existing project primitives (`PollerThread`, `Snapshot`, `IModule`/`IProvider`).

## Global Constraints

- **No unit test framework exists in this codebase.** Every task's verification step is "build succeeds" (MSBuild) unless stated otherwise; the final task also does a live Rainmeter runtime check, matching how every prior metric (GPU/CPU/Memory/Network string+numeric additions) was verified in this project.
- IPv4 only (`GetAddrInfoW`/`IcmpSendEcho`) — no IPv6.
- Packet-loss ring buffer fixed at 20 entries — not configurable.
- ICMP timeout fixed at `min(pingIntervalMs, 1000)` ms — not a separate configurable option.
- Timeout/unreachable/DNS-failure all report `Rtt = 9999` — one failure path, no split.
- No new third-party dependencies — Win32/Winsock APIs only.
- MSBuild lives at `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe` (VS2022, folder literally named `18`). In the Bash tool, prefix with `MSYS2_ARG_CONV_EXCL="*"` so `/`-flags aren't mangled.
- Deployment: Rainmeter must be stopped (`Stop-Process -Name Rainmeter -Force`) before building, or the post-build DLL copy fails with a file-lock error; restart it after (`Start-Process "$env:ProgramFiles\Rainmeter\Rainmeter.exe"`) — use the `PowerShell` tool for these, not `Bash`, since `$env:` doesn't expand under Git Bash.
- Never use `Rainmeter.exe [!bang]` to test — it spawns a duplicate process in this environment. Edit the `.ini` skin directly, then do a clean stop/start.

---

### Task 1: Foundation types — `Category::Ping`, `PingMetric`, `MetricParser`, `IModule`/`IProvider` hook

**Files:**
- Modify: `source/Types/Category.h`
- Create: `source/Types/PingMetric.h`
- Modify: `source/Types/MetricParser.cpp`
- Modify: `source/Core/IModule.h`
- Modify: `source/Core/IProvider.h`
- Modify: `NativeHardwareMonitor.vcxproj`
- Modify: `NativeHardwareMonitor.vcxproj.filters`

**Interfaces:**
- Produces: `Category::Ping` enumerator; `enum class PingMetric : uint32_t { Rtt, PacketLoss, Unknown }`; `ParseCategory(L"ping") -> Category::Ping`; `ParseMetric(Category::Ping, L"rtt"/L"packetloss")`; `IModule::ResolveTarget(const std::wstring& host, uint32_t intervalMs) -> uint32_t` (default returns `0`); same signature added to `IProvider`.

- [ ] **Step 1: Add `Ping` to the `Category` enum**

Edit `source/Types/Category.h`:

```cpp
#pragma once

enum class Category
{
    GPU,
    CPU,
    Memory,
    Network,
    Storage,
    Ping,
    Unknown
};
```

- [ ] **Step 2: Create `PingMetric.h`**

Create `source/Types/PingMetric.h`:

```cpp
#pragma once

#include <cstdint>

enum class PingMetric : uint32_t
{
    Rtt,
    PacketLoss,
    Unknown
};
```

- [ ] **Step 3: Wire `Category::Ping` and `PingMetric` into `MetricParser.cpp`**

In `source/Types/MetricParser.cpp`, add the include near the top (alongside the other `Types/*Metric.h` includes):

```cpp
#include "Types/PingMetric.h"
```

In `ParseCategory`, add before `return Category::Unknown;`:

```cpp
    if (str == L"ping")    return Category::Ping;
```

In `ParseMetric`'s `switch`, add a new case (placement doesn't matter, put it after the `Category::Network` case and before `Category::Storage`):

```cpp
    case Category::Ping:
        if (str == L"rtt")        return static_cast<uint32_t>(PingMetric::Rtt);
        if (str == L"packetloss") return static_cast<uint32_t>(PingMetric::PacketLoss);
        return static_cast<uint32_t>(PingMetric::Unknown);
```

- [ ] **Step 4: Add the `ResolveTarget` hook to `IModule`**

Edit `source/Core/IModule.h`:

```cpp
#pragma once

#include <cstdint>
#include <string>

class IModule
{
public:
    virtual ~IModule() = default;

    virtual bool     Initialize() = 0;
    virtual void     GatherAll() = 0;
    virtual double   GetValue(uint32_t metricId, uint32_t deviceIndex) = 0;
    virtual bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) = 0;
    virtual uint32_t GetDeviceCount() const = 0;

    // Only meaningful for Category::Ping — resolves a target host string to a
    // stable device-index slot (creating one on first sight). Every other
    // module inherits this default and is unaffected.
    virtual uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) { return 0; }
};
```

- [ ] **Step 5: Add the same hook to `IProvider`**

Edit `source/Core/IProvider.h`:

```cpp
#pragma once

#include <cstdint>
#include <string>

#include "Types/Snapshot.h"

class IProvider
{
public:
    virtual ~IProvider() = default;

    virtual bool     Initialize() = 0;
    virtual uint32_t GetDeviceCount() const = 0;
    virtual void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) = 0;
    virtual bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) = 0;

    // See IModule::ResolveTarget — same default-no-op shape one layer down.
    virtual uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) { return 0; }
};
```

- [ ] **Step 6: Add `PingMetric.h` to the Visual Studio project**

In `NativeHardwareMonitor.vcxproj`, find this line in the Header Files `ItemGroup`:

```xml
    <ClInclude Include="source\Types\StorageMetric.h" />
```

Add immediately after it:

```xml
    <ClInclude Include="source\Types\PingMetric.h" />
```

- [ ] **Step 7: Add `PingMetric.h` to the filters file**

In `NativeHardwareMonitor.vcxproj.filters`, find:

```xml
    <ClInclude Include="source\Types\StorageMetric.h">
      <Filter>Header Files\Types</Filter>
    </ClInclude>
```

Add immediately after it:

```xml
    <ClInclude Include="source\Types\PingMetric.h">
      <Filter>Header Files\Types</Filter>
    </ClInclude>
```

- [ ] **Step 8: Build to verify**

```bash
cd "F:/git_repositories/NativeHardwareMonitor"
MSYS2_ARG_CONV_EXCL="*" "/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" NativeHardwareMonitor.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```
Expected: build succeeds (the DLL-copy step will fail with "file in use" if Rainmeter is running — that's fine for this task, stop Rainmeter first if you want a clean run, it's not required until Task 5's live check).

- [ ] **Step 9: Commit**

```bash
git add source/Types/Category.h source/Types/PingMetric.h source/Types/MetricParser.cpp source/Core/IModule.h source/Core/IProvider.h NativeHardwareMonitor.vcxproj NativeHardwareMonitor.vcxproj.filters
git commit -m "Add Ping category foundation types and ResolveTarget interface hook"
```

---

### Task 2: `WinApiPingProvider` — resolution, ICMP echo, background polling

**Files:**
- Create: `source/Providers/WinApi/WinApiPingProvider.h`
- Create: `source/Providers/WinApi/WinApiPingProvider.cpp`
- Modify: `NativeHardwareMonitor.vcxproj`
- Modify: `NativeHardwareMonitor.vcxproj.filters`

**Interfaces:**
- Consumes: `PingMetric` (Task 1), `Core/PollerThread.h`'s `PollerThread` (`Start(uint32_t intervalMs, std::function<void()> callback)`, `Stop()`), `Core/IProvider.h`'s `IProvider` base, `Types/Snapshot.h`'s `Snapshot::Set(uint32_t, double)`.
- Produces: `class WinApiPingProvider : public IProvider` with `Initialize()`, `GetDeviceCount() const`, `GatherSnapshot(uint32_t, Snapshot&)`, `GetString(uint32_t, uint32_t, std::wstring&)` (always returns `false`), and `ResolveTarget(const std::wstring& host, uint32_t intervalMs) -> uint32_t`.

- [ ] **Step 1: Verify the ICMP API shapes before writing code**

```bash
grep -n "IcmpSendEcho\|IcmpCreateFile\|IcmpCloseHandle\|ICMP_ECHO_REPLY\|IP_SUCCESS" "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um/icmpapi.h" "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared/ipexport.h"
```
Expected: confirms `IcmpCreateFile()`, `IcmpSendEcho(HANDLE, IPAddr, LPVOID, WORD, PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD)`, `IcmpCloseHandle(HANDLE)`, the `ICMP_ECHO_REPLY` struct (`Status`, `RoundTripTime` fields), and `IP_SUCCESS == 0` — matches what Step 2/3 below assume. If any signature differs, adjust Step 3's code to match what you find (same rigor this project already applied to `MIB_IF_ROW2` for the Network metrics).

- [ ] **Step 2: Write `WinApiPingProvider.h`**

Create `source/Providers/WinApi/WinApiPingProvider.h`:

```cpp
#pragma once

#include "Core/IProvider.h"
#include "Core/PollerThread.h"
#include "Types/Snapshot.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <icmpapi.h>

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class WinApiPingProvider : public IProvider
{
public:
    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;
    uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) override;

private:
    static constexpr size_t kHistorySize = 20;

    struct Target
    {
        ~Target();

        std::wstring host;
        bool         resolved = false;
        IPAddr       address  = 0;

        HANDLE                        icmpHandle = nullptr;
        uint32_t                      intervalMs = 1000;
        std::unique_ptr<PollerThread> poller;

        std::mutex       mutex;      // guards the four fields below
        double            rtt        = 9999.0;
        double            packetLoss = 0.0;
        std::deque<bool>  history;   // true = reply received, capped at kHistorySize
    };

    void PingOnce(Target* target);

    std::vector<std::unique_ptr<Target>> m_targets;
    mutable std::mutex                    m_targetsMutex; // guards m_targets itself (not Target internals)
};
```

- [ ] **Step 3: Write `WinApiPingProvider.cpp`**

Create `source/Providers/WinApi/WinApiPingProvider.cpp`:

```cpp
#include "Providers/WinApi/WinApiPingProvider.h"
#include "Types/PingMetric.h"
#include "Utils/Debug.h"

#include <algorithm>
#include <cwctype>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

WinApiPingProvider::Target::~Target()
{
    // Stop the poller before closing the handle it's using — PollerThread::Stop()
    // joins the thread, so PingOnce() cannot still be running once this returns.
    if (poller)
        poller->Stop();

    if (icmpHandle)
        IcmpCloseHandle(icmpHandle);
}

static std::wstring NormalizeHost(const std::wstring& input)
{
    std::wstring str = input;

    str.erase(str.begin(),
        std::find_if(str.begin(), str.end(), [](wchar_t c) { return !std::iswspace(c); }));

    str.erase(
        std::find_if(str.rbegin(), str.rend(), [](wchar_t c) { return !std::iswspace(c); }).base(),
        str.end());

    std::transform(str.begin(), str.end(), str.begin(), std::towlower);

    return str;
}

bool WinApiPingProvider::Initialize()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        LOG_STARTUP(L"WinApiPingProvider: WSAStartup failed");
        return false;
    }

    LOG_STARTUP(L"WinApiPingProvider: initialized");
    return true;
}

uint32_t WinApiPingProvider::GetDeviceCount() const
{
    std::lock_guard<std::mutex> lock(m_targetsMutex);
    return static_cast<uint32_t>(m_targets.size());
}

void WinApiPingProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    Target* target = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_targetsMutex);
        if (deviceIndex >= m_targets.size())
            return;
        target = m_targets[deviceIndex].get();
    }

    std::lock_guard<std::mutex> lock(target->mutex);
    snap.Set(static_cast<uint32_t>(PingMetric::Rtt),        target->rtt);
    snap.Set(static_cast<uint32_t>(PingMetric::PacketLoss), target->packetLoss);
}

bool WinApiPingProvider::GetString(uint32_t, uint32_t, std::wstring&)
{
    return false;
}

void WinApiPingProvider::PingOnce(Target* target)
{
    if (!target->resolved)
        return;

    if (!target->icmpHandle)
    {
        target->icmpHandle = IcmpCreateFile();
        if (target->icmpHandle == INVALID_HANDLE_VALUE)
        {
            target->icmpHandle = nullptr;
            return;
        }
    }

    char sendData[32] = "NativeHardwareMonitor ping";
    BYTE replyBuffer[sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8];

    DWORD timeout = std::min(target->intervalMs, 1000u);

    DWORD result = IcmpSendEcho(
        target->icmpHandle,
        target->address,
        sendData, static_cast<WORD>(sizeof(sendData)),
        nullptr,
        replyBuffer, sizeof(replyBuffer),
        timeout);

    bool   success = false;
    double rttMs   = 9999.0;

    if (result != 0)
    {
        auto* reply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer);
        if (reply->Status == IP_SUCCESS)
        {
            success = true;
            rttMs   = static_cast<double>(reply->RoundTripTime);
        }
    }

    std::lock_guard<std::mutex> lock(target->mutex);

    target->history.push_back(success);
    if (target->history.size() > kHistorySize)
        target->history.pop_front();

    size_t misses = 0;
    for (bool hit : target->history)
        if (!hit) ++misses;

    target->rtt        = success ? rttMs : 9999.0;
    target->packetLoss = target->history.empty() ? 0.0
                        : (static_cast<double>(misses) / target->history.size()) * 100.0;
}

uint32_t WinApiPingProvider::ResolveTarget(const std::wstring& host, uint32_t intervalMs)
{
    std::wstring key = NormalizeHost(host);
    uint32_t     interval = intervalMs > 0 ? intervalMs : 1000;

    std::lock_guard<std::mutex> lock(m_targetsMutex);

    for (size_t i = 0; i < m_targets.size(); ++i)
    {
        if (m_targets[i]->host != key)
            continue;

        if (interval < m_targets[i]->intervalMs)
        {
            m_targets[i]->intervalMs = interval;
            Target* t = m_targets[i].get();
            m_targets[i]->poller->Start(interval, [this, t]() { PingOnce(t); });
        }

        return static_cast<uint32_t>(i);
    }

    auto target = std::make_unique<Target>();
    target->host       = key;
    target->intervalMs = interval;

    addrinfo hints{};
    hints.ai_family = AF_INET;

    addrinfo* result = nullptr;
    if (GetAddrInfoW(key.c_str(), nullptr, &hints, &result) == 0 && result)
    {
        auto* sin = reinterpret_cast<sockaddr_in*>(result->ai_addr);
        target->address  = sin->sin_addr.s_addr;
        target->resolved = true;
        FreeAddrInfoW(result);
    }
    else
    {
        LOG_STARTUP(L"WinApiPingProvider: failed to resolve host '%s'", key.c_str());
    }

    uint32_t index = static_cast<uint32_t>(m_targets.size());

    Target* t = target.get();
    target->poller = std::make_unique<PollerThread>();
    target->poller->Start(target->intervalMs, [this, t]() { PingOnce(t); });

    m_targets.push_back(std::move(target));

    LOG_STARTUP(L"WinApiPingProvider: registered target '%s' -> slot %u (resolved=%d)",
        key.c_str(), index, t->resolved ? 1 : 0);

    return index;
}
```

- [ ] **Step 4: Add the new files to the Visual Studio project**

In `NativeHardwareMonitor.vcxproj`, find:

```xml
    <ClCompile Include="source\Providers\WinApi\WinApiNetworkProvider.cpp" />
    <ClCompile Include="source\Providers\WinApi\WinApiStorageProvider.cpp" />
```

Replace with:

```xml
    <ClCompile Include="source\Providers\WinApi\WinApiNetworkProvider.cpp" />
    <ClCompile Include="source\Providers\WinApi\WinApiPingProvider.cpp" />
    <ClCompile Include="source\Providers\WinApi\WinApiStorageProvider.cpp" />
```

And find:

```xml
    <ClInclude Include="source\Providers\WinApi\WinApiNetworkProvider.h" />
    <ClInclude Include="source\Providers\WinApi\WinApiStorageProvider.h" />
```

Replace with:

```xml
    <ClInclude Include="source\Providers\WinApi\WinApiNetworkProvider.h" />
    <ClInclude Include="source\Providers\WinApi\WinApiPingProvider.h" />
    <ClInclude Include="source\Providers\WinApi\WinApiStorageProvider.h" />
```

- [ ] **Step 5: Add the new files to the filters file**

In `NativeHardwareMonitor.vcxproj.filters`, find:

```xml
    <ClCompile Include="source\Providers\WinApi\WinApiNetworkProvider.cpp">
      <Filter>Source Files\Providers\WinApi</Filter>
    </ClCompile>
```

Add immediately after it:

```xml
    <ClCompile Include="source\Providers\WinApi\WinApiPingProvider.cpp">
      <Filter>Source Files\Providers\WinApi</Filter>
    </ClCompile>
```

And find:

```xml
    <ClInclude Include="source\Providers\WinApi\WinApiNetworkProvider.h">
      <Filter>Header Files\Providers\WinApi</Filter>
    </ClInclude>
```

Add immediately after it:

```xml
    <ClInclude Include="source\Providers\WinApi\WinApiPingProvider.h">
      <Filter>Header Files\Providers\WinApi</Filter>
    </ClInclude>
```

- [ ] **Step 6: Build to verify**

```bash
cd "F:/git_repositories/NativeHardwareMonitor"
MSYS2_ARG_CONV_EXCL="*" "/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" NativeHardwareMonitor.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```
Expected: build succeeds with no errors (unused-code warnings are fine — nothing calls `WinApiPingProvider` yet).

- [ ] **Step 7: Commit**

```bash
git add source/Providers/WinApi/WinApiPingProvider.h source/Providers/WinApi/WinApiPingProvider.cpp NativeHardwareMonitor.vcxproj NativeHardwareMonitor.vcxproj.filters
git commit -m "Add WinApiPingProvider: ICMP echo + per-target background polling"
```

---

### Task 3: `PingResolver` + `PingModule`

**Files:**
- Create: `source/Modules/Ping/PingResolver.h`
- Create: `source/Modules/Ping/PingResolver.cpp`
- Create: `source/Modules/Ping/PingModule.h`
- Create: `source/Modules/Ping/PingModule.cpp`
- Modify: `NativeHardwareMonitor.vcxproj`
- Modify: `NativeHardwareMonitor.vcxproj.filters`

**Interfaces:**
- Consumes: `WinApiPingProvider` (Task 2), `IModule`/`IProvider::ResolveTarget` (Task 1).
- Produces: `class PingModule : public IModule` — same shape as `NetworkModule`, plus `ResolveTarget(const std::wstring&, uint32_t) -> uint32_t` which grows its internal `Snapshot` cache as new targets appear (unlike other modules, `PingModule`'s device count grows at runtime, not just at `Initialize()`).

- [ ] **Step 1: Write `PingResolver.h`**

Create `source/Modules/Ping/PingResolver.h`:

```cpp
#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <memory>
#include <string>

class PingResolver
{
public:
    bool     Initialize();
    uint32_t GetDeviceCount() const;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap);
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out);
    uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs);

private:
    std::unique_ptr<IProvider> m_provider;
};
```

- [ ] **Step 2: Write `PingResolver.cpp`**

Create `source/Modules/Ping/PingResolver.cpp`:

```cpp
#include "Modules/Ping/PingResolver.h"
#include "Providers/WinApi/WinApiPingProvider.h"
#include "Utils/Debug.h"

bool PingResolver::Initialize()
{
    auto provider = std::make_unique<WinApiPingProvider>();

    if (!provider->Initialize())
    {
        LOG_STARTUP(L"PingResolver: WinApiPingProvider initialization failed");
        return false;
    }

    LOG_STARTUP(L"PingResolver: WinApiPingProvider initialized");
    m_provider = std::move(provider);

    return true;
}

uint32_t PingResolver::GetDeviceCount() const
{
    return m_provider ? m_provider->GetDeviceCount() : 0;
}

void PingResolver::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (m_provider)
        m_provider->GatherSnapshot(deviceIndex, snap);
}

bool PingResolver::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_provider ? m_provider->GetString(metricId, deviceIndex, out) : false;
}

uint32_t PingResolver::ResolveTarget(const std::wstring& host, uint32_t intervalMs)
{
    return m_provider ? m_provider->ResolveTarget(host, intervalMs) : 0;
}
```

- [ ] **Step 3: Write `PingModule.h`**

Create `source/Modules/Ping/PingModule.h`:

```cpp
#pragma once

#include "Core/IModule.h"
#include "Types/Snapshot.h"

#include <memory>
#include <vector>

#include "Modules/Ping/PingResolver.h"

class PingModule : public IModule
{
public:
    bool     Initialize() override;
    void     GatherAll() override;
    double   GetValue(uint32_t metricId, uint32_t deviceIndex) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;
    uint32_t GetDeviceCount() const override;
    uint32_t ResolveTarget(const std::wstring& host, uint32_t intervalMs) override;

private:
    std::unique_ptr<PingResolver> m_resolver;
    std::vector<Snapshot>         m_snapshots;
    bool m_initialized = false;
};
```

- [ ] **Step 4: Write `PingModule.cpp`**

Create `source/Modules/Ping/PingModule.cpp`:

```cpp
#include "Modules/Ping/PingModule.h"
#include "Modules/Ping/PingResolver.h"

bool PingModule::Initialize()
{
    if (m_initialized)
        return true;

    m_resolver = std::make_unique<PingResolver>();

    if (!m_resolver->Initialize())
        return false;

    // No m_snapshots.resize() here, unlike other modules — Ping starts with
    // zero targets; ResolveTarget() grows m_snapshots as hosts are registered.
    m_initialized = true;
    return true;
}

void PingModule::GatherAll()
{
    if (!m_initialized)
        return;

    uint32_t count = m_resolver->GetDeviceCount();
    if (m_snapshots.size() < count)
        m_snapshots.resize(count);

    for (uint32_t i = 0; i < count; ++i)
        m_resolver->GatherSnapshot(i, m_snapshots[i]);
}

double PingModule::GetValue(uint32_t metricId, uint32_t deviceIndex)
{
    if (deviceIndex >= m_snapshots.size())
        return -2.0;

    return m_snapshots[deviceIndex].Get(metricId);
}

bool PingModule::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    return m_resolver ? m_resolver->GetString(metricId, deviceIndex, out) : false;
}

uint32_t PingModule::GetDeviceCount() const
{
    return m_resolver ? m_resolver->GetDeviceCount() : 0;
}

uint32_t PingModule::ResolveTarget(const std::wstring& host, uint32_t intervalMs)
{
    if (!m_resolver)
        return 0;

    uint32_t index = m_resolver->ResolveTarget(host, intervalMs);

    if (m_snapshots.size() <= index)
        m_snapshots.resize(index + 1);

    return index;
}
```

- [ ] **Step 5: Add the new files and filter to the Visual Studio project**

In `NativeHardwareMonitor.vcxproj`, find:

```xml
    <ClCompile Include="source\Modules\Storage\StorageModule.cpp" />
    <ClCompile Include="source\Modules\Storage\StorageResolver.cpp" />
```

Add immediately after it:

```xml
    <ClCompile Include="source\Modules\Ping\PingModule.cpp" />
    <ClCompile Include="source\Modules\Ping\PingResolver.cpp" />
```

Find:

```xml
    <ClInclude Include="source\Modules\Storage\StorageModule.h" />
    <ClInclude Include="source\Modules\Storage\StorageResolver.h" />
```

Add immediately after it:

```xml
    <ClInclude Include="source\Modules\Ping\PingModule.h" />
    <ClInclude Include="source\Modules\Ping\PingResolver.h" />
```

- [ ] **Step 6: Add the new filter and file entries to the filters file**

In `NativeHardwareMonitor.vcxproj.filters`, find the filter declarations block and add a new filter after the `Storage` one:

```xml
    <Filter Include="Source Files\Modules\Storage">
      <UniqueIdentifier>{a1b2c3d4-0001-0001-0001-000000000008}</UniqueIdentifier>
    </Filter>
```
becomes:
```xml
    <Filter Include="Source Files\Modules\Storage">
      <UniqueIdentifier>{a1b2c3d4-0001-0001-0001-000000000008}</UniqueIdentifier>
    </Filter>
    <Filter Include="Source Files\Modules\Ping">
      <UniqueIdentifier>{a1b2c3d4-0001-0001-0001-000000000019}</UniqueIdentifier>
    </Filter>
```

Find the Modules\Storage ClCompile block:

```xml
    <ClCompile Include="source\Modules\Storage\StorageModule.cpp">
      <Filter>Source Files\Modules\Storage</Filter>
    </ClCompile>
    <ClCompile Include="source\Modules\Storage\StorageResolver.cpp">
      <Filter>Source Files\Modules\Storage</Filter>
    </ClCompile>
```

Add immediately after it:

```xml
    <!-- Modules\Ping -->
    <ClCompile Include="source\Modules\Ping\PingModule.cpp">
      <Filter>Source Files\Modules\Ping</Filter>
    </ClCompile>
    <ClCompile Include="source\Modules\Ping\PingResolver.cpp">
      <Filter>Source Files\Modules\Ping</Filter>
    </ClCompile>
```

Find the header filter declarations block, add after `Header Files\Modules\Storage`:

```xml
    <Filter Include="Header Files\Modules\Storage">
      <UniqueIdentifier>{b1b2c3d4-0001-0001-0001-000000000008}</UniqueIdentifier>
    </Filter>
```
becomes:
```xml
    <Filter Include="Header Files\Modules\Storage">
      <UniqueIdentifier>{b1b2c3d4-0001-0001-0001-000000000008}</UniqueIdentifier>
    </Filter>
    <Filter Include="Header Files\Modules\Ping">
      <UniqueIdentifier>{b1b2c3d4-0001-0001-0001-000000000018}</UniqueIdentifier>
    </Filter>
```

Find the Modules\Storage ClInclude block:

```xml
    <ClInclude Include="source\Modules\Storage\StorageModule.h">
      <Filter>Header Files\Modules\Storage</Filter>
    </ClInclude>
    <ClInclude Include="source\Modules\Storage\StorageResolver.h">
      <Filter>Header Files\Modules\Storage</Filter>
    </ClInclude>
```

Add immediately after it:

```xml
    <!-- Modules\Ping -->
    <ClInclude Include="source\Modules\Ping\PingModule.h">
      <Filter>Header Files\Modules\Ping</Filter>
    </ClInclude>
    <ClInclude Include="source\Modules\Ping\PingResolver.h">
      <Filter>Header Files\Modules\Ping</Filter>
    </ClInclude>
```

- [ ] **Step 7: Build to verify**

```bash
cd "F:/git_repositories/NativeHardwareMonitor"
MSYS2_ARG_CONV_EXCL="*" "/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" NativeHardwareMonitor.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```
Expected: build succeeds. `PingModule`/`PingResolver` aren't referenced by `HardwareCore` yet, so still no runtime behavior change.

- [ ] **Step 8: Commit**

```bash
git add source/Modules/Ping NativeHardwareMonitor.vcxproj NativeHardwareMonitor.vcxproj.filters
git commit -m "Add PingResolver/PingModule delegate layer"
```

---

### Task 4: Wire `Category::Ping` into `HardwareCore` and `Plugin.cpp`

**Files:**
- Modify: `source/Core/HardwareCore.h`
- Modify: `source/Core/HardwareCore.cpp`
- Modify: `source/Plugin/Plugin.h`
- Modify: `source/Plugin/Plugin.cpp`

**Interfaces:**
- Consumes: `PingModule` (Task 3), `ParseCategory`/`ParseMetric` (Task 1).
- Produces: `HardwareCore::RegisterMeasure(Category, uint32_t metricId, uint32_t deviceIndex, uint32_t updateOverrideMs, const std::wstring& targetHost = L"", uint32_t pingIntervalMs = 0) -> uint32_t` — for `Category::Ping`, `deviceIndex` is ignored and replaced by whatever `ResolveTarget` returns.

- [ ] **Step 1: Register `PingModule` and extend `RegisterMeasure`'s signature in `HardwareCore.h`**

Edit `source/Core/HardwareCore.h`, change:

```cpp
    uint32_t RegisterMeasure(Category category, uint32_t metricId, uint32_t deviceIndex, uint32_t updateOverrideMs);
```

to:

```cpp
    uint32_t RegisterMeasure(
        Category             category,
        uint32_t              metricId,
        uint32_t              deviceIndex,
        uint32_t              updateOverrideMs,
        const std::wstring&  targetHost     = L"",
        uint32_t              pingIntervalMs = 0);
```

- [ ] **Step 2: Register the module and implement the new logic in `HardwareCore.cpp`**

Edit `source/Core/HardwareCore.cpp`, add the include:

```cpp
#include "Modules/Ping/PingModule.h"
```

In the constructor, add the new module (order doesn't matter — placed after Storage to match the enum's declaration order):

```cpp
    m_modules[Category::GPU]     = make(std::make_unique<GpuModule>());
    m_modules[Category::CPU]     = make(std::make_unique<CpuModule>());
    m_modules[Category::Memory]  = make(std::make_unique<MemoryModule>());
    m_modules[Category::Network] = make(std::make_unique<NetworkModule>());
    m_modules[Category::Storage] = make(std::make_unique<StorageModule>());
    m_modules[Category::Ping]    = make(std::make_unique<PingModule>());
```

Replace the whole `RegisterMeasure` definition:

```cpp
uint32_t HardwareCore::RegisterMeasure(
    Category            category,
    uint32_t             metricId,
    uint32_t             deviceIndex,
    uint32_t             updateOverrideMs,
    const std::wstring& targetHost,
    uint32_t             pingIntervalMs)
{
    uint32_t handle = m_nextHandle++;

    if (category == Category::Ping)
    {
        // Ping devices are resolved from a host string, not a skin-supplied
        // Device= index — and unlike other categories, it must be initialized
        // here (at registration time) rather than lazily on first read, since
        // the background ping polling has to start the moment the skin loads.
        ModuleState* state = GetModuleState(category);
        if (state && state->module)
        {
            if (!state->initialized)
                state->initialized = state->module->Initialize();

            if (state->initialized)
                deviceIndex = state->module->ResolveTarget(targetHost, pingIntervalMs);
        }
    }

    m_measures.emplace(handle, MeasureEntry{ category, metricId, deviceIndex, updateOverrideMs });

    if (updateOverrideMs > 0)
    {
        ModuleState* state = GetModuleState(category);
        if (state)
        {
            state->overrides[handle] = updateOverrideMs;
            UpdatePoller(*state);
        }
    }

    return handle;
}
```

- [ ] **Step 3: Add `Host`/`pingIntervalMs` fields to `MeasureContext`**

Edit `source/Plugin/Plugin.h`:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include "Types/Category.h"

struct MeasureContext
{
    bool valid = false;

    Category  category         = Category::Unknown;
    uint32_t  metricId         = 0;
    uint32_t  deviceIndex      = 0;
    uint32_t  handle           = 0;
    int       debugLevel       = 0;
    uint32_t  updateOverrideMs = 0;
    uint32_t  pingIntervalMs   = 1000;

    std::wstring categoryStr;
    std::wstring metricStr;
    std::wstring host;
};
```

- [ ] **Step 4: Read `Host=`/`PingInterval=` and pass them through in `Plugin.cpp`**

Edit `source/Plugin/Plugin.cpp`, replace `ReadContext`:

```cpp
static void ReadContext(MeasureContext* ctx, void* rm)
{
    // Each RmReadString returns a pointer into Rainmeter's internal buffer —
    // assign to wstring immediately before the next call overwrites it.
    ctx->categoryStr = RmReadString(rm, L"Category", L"", FALSE);
    ctx->metricStr   = RmReadString(rm, L"Metric",   L"", FALSE);
    ctx->host        = RmReadString(rm, L"Host",     L"", FALSE);

    ctx->category         = ParseCategory(ctx->categoryStr);
    ctx->metricId         = ParseMetric(ctx->category, ctx->metricStr);
    ctx->deviceIndex      = static_cast<uint32_t>(RmReadInt(rm, L"Device", 0));
    ctx->debugLevel       = RmReadInt(rm, L"Debug", 0);
    ctx->updateOverrideMs = static_cast<uint32_t>(RmReadInt(rm, L"UpdateOverride", 0));
    ctx->pingIntervalMs   = static_cast<uint32_t>(RmReadInt(rm, L"PingInterval", 1000));
}
```

In `Initialize`, replace the `RegisterMeasure` call:

```cpp
    ctx->handle = GetCore().RegisterMeasure(
        ctx->category,
        ctx->metricId,
        ctx->deviceIndex,
        ctx->updateOverrideMs,
        ctx->host,
        ctx->pingIntervalMs
    );
```

In `Reload`, replace its `RegisterMeasure` call the same way:

```cpp
    ctx->handle = GetCore().RegisterMeasure(
        ctx->category,
        ctx->metricId,
        ctx->deviceIndex,
        ctx->updateOverrideMs,
        ctx->host,
        ctx->pingIntervalMs
    );
```

- [ ] **Step 5: Build to verify**

```bash
cd "F:/git_repositories/NativeHardwareMonitor"
MSYS2_ARG_CONV_EXCL="*" "/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" NativeHardwareMonitor.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```
Expected: build succeeds. The feature is now fully wired end-to-end — a skin with `Category=Ping, Host=..., Metric=rtt` will register and start pinging, but no test skin uses it yet.

- [ ] **Step 6: Commit**

```bash
git add source/Core/HardwareCore.h source/Core/HardwareCore.cpp source/Plugin/Plugin.h source/Plugin/Plugin.cpp
git commit -m "Wire Category::Ping into HardwareCore registration and Plugin.cpp options"
```

---

### Task 5: Test skin + live verification

**Files:**
- Modify: `C:\Users\Admin\Documents\Rainmeter\Skins\plugin_test\network.ini` (outside the repo — Rainmeter skins folder, not tracked in git)

**Interfaces:**
- Consumes: the fully-wired `Category=Ping` plugin path from Task 4.
- Produces: none (this is the runtime verification task; no new code interfaces).

- [ ] **Step 1: Replace the `Measure_Ping` stub with a real Ping measure**

In `network.ini`, replace:

```ini
[Measure_Ping] ;    [TO IMPLEMENT]
Measure=String
String=0
```

with:

```ini
[Measure_Ping]
Measure=Plugin
Plugin=#plugin_name#
Category=Ping
Metric=rtt
Host=8.8.8.8
Debug=2

[Measure_PacketLoss]
Measure=Plugin
Plugin=#plugin_name#
Category=Ping
Metric=packetloss
Host=8.8.8.8
Debug=2
```

- [ ] **Step 2: Add a Packet Loss meter next to the existing Ping meter**

Find:

```ini
[Meter_Ping_Value]
Meter=String
MeterStyle=Text
MeasureName=Measure_Ping
StringAlign=Right
Text=%1
Postfix=" ms"
X=([Background:W] - 10)
Y=0r
```

Add immediately after it:

```ini
[Meter_PacketLoss]
Meter=String
MeterStyle=Text
Text=Packet Loss:
X=10
Y=5R

[Meter_PacketLoss_Value]
Meter=String
MeterStyle=Text
MeasureName=Measure_PacketLoss
StringAlign=Right
Text=%1
Postfix=" %"
X=([Background:W] - 10)
Y=0r
```

- [ ] **Step 3: Update the option-comment header for completeness**

Find:

```ini
;   [Measure]
;   Measure=Plugin
;   Plugin=NativeHardwareMonitor
;   Category=Network
;   Metric={value}  [Upload, Download, UploadTotal, DownloadTotal, Speed, ReceiveLinkSpeed,
;                    PacketsReceived, PacketsSent, ErrorsReceived, ErrorsSent,
;                    DiscardsReceived, DiscardsSent, Mtu,
;                    Alias, Description, PhysicalAddress, ConnectionStatus]
;   Device={value}  [0, 1, 2, etc] which is [WiFi, Ethernet, etc]
;   Debug={value}  [1, 0]
```

Add a second block right after it:

```ini
;
;   [Measure] (Ping — separate Category, Device= is unused)
;   Measure=Plugin
;   Plugin=NativeHardwareMonitor
;   Category=Ping
;   Metric={value}  [Rtt, PacketLoss]
;   Host={value}  hostname or IP to ping (required)
;   PingInterval={value}  ms between pings for this host, default 1000
;   Debug={value}  [1, 0]
```

- [ ] **Step 4: Stop Rainmeter, build+deploy, restart**

```powershell
Stop-Process -Name Rainmeter -Force -ErrorAction SilentlyContinue
```

```bash
cd "F:/git_repositories/NativeHardwareMonitor"
MSYS2_ARG_CONV_EXCL="*" "/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" NativeHardwareMonitor.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```
Expected: `1 file(s) copied` for the post-build DLL copy (not a file-lock error).

```powershell
Start-Process "$env:ProgramFiles\Rainmeter\Rainmeter.exe"
```

- [ ] **Step 5: Verify live via the Rainmeter log**

Flip `Logging=1` in `Rainmeter.ini` (`%APPDATA%\Rainmeter\Rainmeter.ini`), restart Rainmeter (stop/start as in Step 4, not the `[!bang]` CLI), then check the log:

```bash
grep -i "WinApiPingProvider\|PingResolver" "/c/Users/Admin/AppData/Roaming/Rainmeter/Rainmeter.log"
```

Expected: lines like `PingResolver: WinApiPingProvider initialized` and `WinApiPingProvider: registered target '8.8.8.8' -> slot 0 (resolved=1)`.

Then check the measure itself is producing real numbers, not stuck at a sentinel:

```bash
grep -i "Measure_Ping\|Measure_PacketLoss" "/c/Users/Admin/AppData/Roaming/Rainmeter/Rainmeter.log"
```

Expected: `Registered (handle=N)` lines for both measures, and (since `Debug=2` logs every `Update()`) `Rtt` values that settle to a real millisecond number within a few seconds (not permanently `9999`), with `PacketLoss` at `0` (or a small non-100 number if the network hiccups during the check).

Revert `Logging=1` back to `0` in `Rainmeter.ini` and do one final stop/start once you've confirmed it — don't leave verbose logging on.

- [ ] **Step 6: Commit**

The skin file lives outside the git repo (Rainmeter skins folder), so there's nothing in `F:/git_repositories/NativeHardwareMonitor` to commit for this task. If the repo has its own copy of example skins tracked elsewhere, skip this step — confirm with `git status` first.

```bash
cd "F:/git_repositories/NativeHardwareMonitor"
git status
```
Expected: clean (all source changes were already committed in Tasks 1–4).

---

## Post-plan cleanup

Update project memory (`project_v2_network_metrics.md` mentioned Ping as a placeholder — no memory file currently exists for Ping specifically) with a new `project_v2_ping_metrics.md` covering: the `Category::Ping` architecture decision, the 9999 sentinel convention, the always-background-thread model, and that Storage is still the next unexpanded category after this.
