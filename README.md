# NativeHardwareMonitor

NativeHardwareMonitor (NHM) is a **Rainmeter plugin** that retrieves hardware metrics directly from **Windows APIs and hardware drivers**.  
It does not require external monitoring software such as OpenHardwareMonitor or HWiNFO.

The plugin is designed to provide **lightweight and direct hardware monitoring** for Rainmeter skins.

---

> [!WARNING]
> **AI ASSISTED DEVELOPMENT**
>
> This project was developed with the assistance of AI tools.
>
> AI systems were used to help with:
> - code generation
> - debugging assistance
> - documentation writing
> - architectural suggestions
>
> All code, design decisions, and final implementations were reviewed and validated by the project author.
>
> AI-generated output may occasionally contain mistakes or inefficient implementations.
> If you encounter any issues, please report them.


---


## Features

- Native Rainmeter plugin
- Direct access to Windows APIs and GPU drivers
- No dependency on external monitoring tools
- Modular backend architecture
- Shared backend instance for efficiency
- Designed for low overhead
- Extensible provider system

---

## Supported Hardware Categories

| Category | Provider |
|--------|--------|
| CPU | Windows API / MSR driver |
| GPU | NVML (NVIDIA) / ADLX (AMD) |
| Memory | Windows API |
| Network | Windows API |
| Storage | Windows API |

---

## Architecture

The plugin uses a modular backend architecture.

```
Rainmeter Measure
      │
      ▼
Plugin.cpp
      │
      ▼
HardwareCore
      │
      ├── CPU Backend
      ├── GPU Backend
      ├── Memory Backend
      ├── Network Backend
      └── Storage Backend
```

Each backend retrieves hardware information using a dedicated provider.

---

## Project Structure

```
source/
├─ Core/
│  ├─ HardwareCore
│  ├─ BaseBackend
│  └─ ICategoryBackend
│
├─ Categories/
│  ├─ CPU/
│  │  ├─ CpuBackend
│  │  └─ WinApiProvider
│  │
│  ├─ GPU/
│  │  ├─ NvidiaProvider (NVML)
│  │  ├─ AmdProvider (ADLX)
│  │  └─ GpuBackend
│  │
│  ├─ Memory/
│  │  └─ WinApiMemoryProvider
│  │
│  ├─ Network/
│  │  └─ WinApiNetworkProvider
│  │
│  └─ Storage/
│     └─ WinApiStorageProvider
│
├─ Types/
│  ├─ Category
│  ├─ CpuMetric
│  ├─ GpuMetric
│  ├─ MemoryMetric
│  ├─ NetworkMetric
│  └─ StorageMetric
│
├─ Utils/
│  ├─ Debug
│  ├─ TimeUtils
│  └─ MsrDriver
│
└─ Plugin/
   └─ Plugin.cpp
```

---

## Installation

Prebuilt binaries are automatically generated using **GitHub Actions**.

### Download Prebuilt Plugin

1. Open the GitHub repository:

   https://github.com/Kurou-kun/NativeHardwareMonitor

2. Navigate to **Actions**

3. Select the latest successful workflow run.

4. Download the **Artifacts** package.

5. Extract the plugin file:

```
NativeHardwareMonitor.dll
```

6. Place the file into the Rainmeter plugins directory:

```
Documents\Rainmeter\Plugins\
```

7. Refresh Rainmeter.

---

## Building from Source

The project uses **CMake** for building.

### Requirements

- Visual Studio 2022
- CMake 3.20+
- Windows 10 / 11 SDK

### Build Steps

Clone the repository:

```
git clone https://github.com/Kurou-kun/NativeHardwareMonitor.git
cd NativeHardwareMonitor
```

Configure the project:

```
cmake -B build -S . -G "Visual Studio 17 2022"
```

Build the plugin:

```
cmake --build build --config Release
```

The compiled plugin will be located in:

```
build/Release/NativeHardwareMonitor.dll
```

Copy the DLL into:

```
Documents\Rainmeter\Plugins\
```

and refresh Rainmeter.

---

## Basic Usage

Example measure:

```ini
[MeasureGPU]
Measure=Plugin
Plugin=NativeHardwareMonitor
Category=GPU
Metric=Load
Device=0
```

Example meter:

```ini
[MeterGPU]
Meter=String
MeasureName=MeasureGPU
Text=GPU Load: %1%
```

---

## Measure Parameters

### Category

Specifies the hardware category.

Supported values:

```
CPU
GPU
Memory
Network
Storage
```

Example:

```
Category=GPU
```

---

### Metric

Defines which metric to retrieve.

Example GPU metrics:

```
Load
Clock
Temp
Fan
Power
VramUsed
VramShared
VramTotal
```

Example:

```
Metric=Load
```

---

### Device

Specifies the hardware device index.

```
Device=0
```

Used when multiple GPUs, disks or network interfaces exist.

---

## Backend Behavior

All measures share a **single backend instance**.

This means:

- hardware polling occurs once
- multiple Rainmeter measures reuse the same values
- performance overhead remains minimal

The polling frequency follows the **Rainmeter skin update rate**.

---

## Requirements

- Windows 10 or Windows 11
- Rainmeter 4.5 or newer
- Compatible GPU drivers (NVIDIA NVML / AMD ADLX)

---

## AI Notice

Parts of this project were developed with the assistance of AI tools.

AI was used mainly for development assistance and documentation generation.  
All code, architecture decisions and implementations were reviewed and validated by the project author.

---

## License

This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0)**.

You are free to:

- **Share** — copy and redistribute the material in any medium or format  
- **Adapt** — remix, transform, and build upon the material  

Under the following terms:

- **Attribution** — You must give appropriate credit to the original author.
- **NonCommercial** — You may not use the material for commercial purposes.
- **ShareAlike** — If you remix, transform, or build upon the material, you must distribute your contributions under the same license.

Full license text:  
https://creativecommons.org/licenses/by-nc-sa/4.0/
