# Installation

Two ways in: drop in a prebuilt DLL, or build it yourself.

## Requirements

- Windows 10/11, **x64**
- [Rainmeter](https://www.rainmeter.net/) (64-bit — the only edition)
- NVIDIA GPU metrics: NVML/NVAPI ship with the GeForce/Studio driver — nothing extra
- AMD GPU metrics: ADLX/ADL2 ship with the Adrenalin driver — nothing extra
- **No** kernel driver, **no** service, **no** elevated install. It's one DLL.

## Option A — drop in the DLL

1. Get `NativeHardwareMonitor.dll` (a release download, or your own build).
2. Copy it into `%APPDATA%\Rainmeter\Plugins\`
   (`C:\Users\<you>\AppData\Roaming\Rainmeter\Plugins\`).
3. Restart Rainmeter.
4. Reference it from any measure with `Plugin=NativeHardwareMonitor`.

That's the whole install. The plugin loads on demand when a skin uses it.

## Option B — build from source

**Toolchain:** Visual Studio 2022, C++17, x64. No CMake — MSBuild against the `.vcxproj` directly.

```powershell
# from the repo root
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
  NativeHardwareMonitor.vcxproj /p:Configuration=Release /p:Platform=x64
```

> The VS2022 install folder here is literally named `18`, not `2022`. If your path
> differs, find it with
> `& "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`.

- **Output:** `x64\Release\NativeHardwareMonitor.dll`
- **Post-build:** the project auto-copies the DLL to `%APPDATA%\Rainmeter\Plugins\` with a plain `copy /Y`.
- **Links against:** `sdk\rainmeter\API\x64\Rainmeter.lib`, `advapi32.lib`, `psapi.lib`, `iphlpapi.lib`.

### Deploying over a running Rainmeter

Rainmeter keeps the DLL loaded, so the post-build copy fails with *"file in use"* while
it's running. Stop it, build, restart:

```powershell
Stop-Process -Name Rainmeter -Force
# ...run the MSBuild command above...
Start-Process "$env:ProgramFiles\Rainmeter\Rainmeter.exe"
```

## Vendored SDKs

Everything needed to build is already in `sdk/` — nothing to fetch.

| Dir | Library | Role |
|---|---|---|
| `sdk/rainmeter/` | Rainmeter plugin SDK | `RainmeterAPI.h` + `Rainmeter.lib` |
| `sdk/nvml/` | NVIDIA Management Library | Primary NVIDIA GPU metrics |
| `sdk/nvapi/` | NVIDIA API | NVIDIA fallback |
| `sdk/adlx/` | AMD Display Library eXtended | Primary AMD GPU metrics |
| `sdk/adl/` | AMD Display Library | Legacy AMD fallback |

Next: **[Configuring Measures](Configuring-Measures)**.
