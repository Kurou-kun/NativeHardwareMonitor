#include "Types/MetricParser.h"

#include "Types/GpuMetric.h"
#include "Types/CpuMetric.h"
#include "Types/MemoryMetric.h"
#include "Types/NetworkMetric.h"
#include "Types/StorageMetric.h"
#include "Types/PingMetric.h"
#include "Types/BatteryMetric.h"
#include "Types/SystemMetric.h"
#include "Types/MotherboardMetric.h"
#include "Types/DisplayMetric.h"

#include <algorithm>
#include <cwctype>

static std::wstring Normalize(const std::wstring& input)
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

// Every canonical name below keeps working; the second (and third) alternates are
// optional short/alias forms. Aliases must stay unique within their category — the
// match is exact per token, category is resolved first so cross-category reuse is fine.
Category ParseCategory(const std::wstring& input)
{
    std::wstring str = Normalize(input);

    if (str == L"gpu" || str == L"graphics")                     return Category::GPU;
    if (str == L"cpu" || str == L"processor")                    return Category::CPU;
    if (str == L"memory" || str == L"mem")                       return Category::Memory;
    if (str == L"network" || str == L"net")                      return Category::Network;
    if (str == L"storage" || str == L"disk" || str == L"drives") return Category::Storage;
    if (str == L"ping")                                          return Category::Ping;
    if (str == L"battery")                                       return Category::Battery;
    if (str == L"system" || str == L"sys")                       return Category::System;
    if (str == L"motherboard" || str == L"mb")                   return Category::Motherboard;
    if (str == L"display" || str == L"monitor" || str == L"screen") return Category::Display;

    return Category::Unknown;
}

std::wstring CategoryName(Category category)
{
    switch (category)
    {
    case Category::GPU:         return L"GPU";
    case Category::CPU:         return L"CPU";
    case Category::Memory:      return L"Memory";
    case Category::Network:     return L"Network";
    case Category::Storage:     return L"Storage";
    case Category::Ping:        return L"Ping";
    case Category::Battery:     return L"Battery";
    case Category::System:      return L"System";
    case Category::Motherboard: return L"Motherboard";
    case Category::Display:     return L"Display";
    default:                    return L"Unknown";
    }
}

uint32_t MetricCount(Category category)
{
    switch (category)
    {
    case Category::GPU:         return static_cast<uint32_t>(GpuMetric::Unknown);
    case Category::CPU:         return static_cast<uint32_t>(CpuMetric::Unknown);
    case Category::Memory:      return static_cast<uint32_t>(MemoryMetric::Unknown);
    case Category::Network:     return static_cast<uint32_t>(NetworkMetric::Unknown);
    case Category::Storage:     return static_cast<uint32_t>(StorageMetric::Unknown);
    case Category::Ping:        return static_cast<uint32_t>(PingMetric::Unknown);
    case Category::Battery:     return static_cast<uint32_t>(BatteryMetric::Unknown);
    case Category::System:      return static_cast<uint32_t>(SystemMetric::Unknown);
    case Category::Motherboard: return static_cast<uint32_t>(MotherboardMetric::Unknown);
    case Category::Display:     return static_cast<uint32_t>(DisplayMetric::Unknown);
    default:                    return 0;
    }
}

// One row per metric: the id, the canonical name a Debug=2 dump prints, and the optional
// short alias a skin may write instead of it. The parse spelling is the canonical name
// lowercased - which is how every category already worked - so the name cannot drift
// from the enum member it belongs to: the macro derives it from the member itself.
//
// This table replaced ten if-chains in ParseMetric plus two switches in MetricName that
// spelled the same strings a second time. tools/metric_parse_check.cpp pins every name
// and alias in it to the id the if-chains returned.
struct MetricDef
{
    uint32_t       id;
    const wchar_t* name;
    const wchar_t* alias;   // nullptr when the metric has no short form
};

#define NHM_WIDEN2(s) L##s
#define NHM_WIDEN(s)  NHM_WIDEN2(s)
#define DEF(T, member, alias) { static_cast<uint32_t>(T::member), NHM_WIDEN(#member), alias }

static const MetricDef GPUDefs[] = {
    DEF(GpuMetric, Usage,              nullptr),
    DEF(GpuMetric, CoreClock,          L"clock"),
    DEF(GpuMetric, EffectiveClock,     L"effclock"),
    DEF(GpuMetric, MemoryClock,        L"memclock"),
    DEF(GpuMetric, Temperature,        L"temp"),
    DEF(GpuMetric, HotspotTemperature, L"hotspot"),
    DEF(GpuMetric, MemoryTemperature,  L"memtemp"),
    DEF(GpuMetric, VramUsed,           L"vram"),
    DEF(GpuMetric, VramTotal,          nullptr),
    DEF(GpuMetric, FanSpeed,           L"fan"),
    DEF(GpuMetric, FanSpeedRPM,        L"fanrpm"),
    DEF(GpuMetric, Power,              nullptr),
    DEF(GpuMetric, PowerLimit,         nullptr),
    DEF(GpuMetric, Voltage,            nullptr),
    DEF(GpuMetric, IntakeTemperature,  L"intaketemp"),
    DEF(GpuMetric, TotalBoardPower,    L"tbp"),
    DEF(GpuMetric, Name,               nullptr),
    DEF(GpuMetric, DriverVersion,      L"driver"),
    DEF(GpuMetric, VbiosVersion,       L"vbios"),
    DEF(GpuMetric, PciDeviceId,        L"pciid"),
    DEF(GpuMetric, MaxCoreClock,       L"maxclock"),
    DEF(GpuMetric, PcieLinkGen,        L"pciegen"),
    DEF(GpuMetric, PcieLinkWidth,      L"pciewidth"),
    DEF(GpuMetric, EncoderUsage,       L"encoder"),
    DEF(GpuMetric, DecoderUsage,       L"decoder"),
    DEF(GpuMetric, PerfState,          L"pstate"),
    DEF(GpuMetric, ThrottleReasons,    L"throttle"),
};

static const MetricDef CPUDefs[] = {
    DEF(CpuMetric, Usage,            nullptr),
    DEF(CpuMetric, Clock,            nullptr),
    DEF(CpuMetric, MaxClock,         nullptr),
    DEF(CpuMetric, Voltage,          nullptr),
    DEF(CpuMetric, Name,             nullptr),
    DEF(CpuMetric, Vendor,           nullptr),
    DEF(CpuMetric, Identifier,       L"id"),
    DEF(CpuMetric, MicrocodeVersion, L"microcode"),
    DEF(CpuMetric, CoreCount,        L"cores"),
    DEF(CpuMetric, ThreadCount,      L"threads"),
    DEF(CpuMetric, CacheL1,          nullptr),
    DEF(CpuMetric, CacheL2,          nullptr),
    DEF(CpuMetric, CacheL3,          nullptr),
    DEF(CpuMetric, Architecture,     L"arch"),
};

static const MetricDef MemoryDefs[] = {
    DEF(MemoryMetric, Used,         nullptr),
    DEF(MemoryMetric, Free,         nullptr),
    DEF(MemoryMetric, Total,        nullptr),
    DEF(MemoryMetric, UsedPercent,  L"usedpct"),
    DEF(MemoryMetric, Cached,       nullptr),
    DEF(MemoryMetric, Speed,        nullptr),
    DEF(MemoryMetric, MemoryType,   L"type"),
    DEF(MemoryMetric, Manufacturer, L"mfr"),
    DEF(MemoryMetric, PartNumber,   L"part"),
    DEF(MemoryMetric, ModuleCount,  L"modules"),
};

static const MetricDef NetworkDefs[] = {
    DEF(NetworkMetric, Download,         nullptr),
    DEF(NetworkMetric, Upload,           nullptr),
    DEF(NetworkMetric, DownloadTotal,    L"dltotal"),
    DEF(NetworkMetric, UploadTotal,      L"ultotal"),
    DEF(NetworkMetric, Speed,            nullptr),
    DEF(NetworkMetric, ReceiveLinkSpeed, L"linkspeed"),
    DEF(NetworkMetric, PacketsReceived,  L"rxpackets"),
    DEF(NetworkMetric, PacketsSent,      L"txpackets"),
    DEF(NetworkMetric, ErrorsReceived,   L"rxerrors"),
    DEF(NetworkMetric, ErrorsSent,       L"txerrors"),
    DEF(NetworkMetric, DiscardsReceived, L"rxdiscards"),
    DEF(NetworkMetric, DiscardsSent,     L"txdiscards"),
    DEF(NetworkMetric, Mtu,              nullptr),
    DEF(NetworkMetric, Alias,            nullptr),
    DEF(NetworkMetric, Description,      L"desc"),
    DEF(NetworkMetric, PhysicalAddress,  L"mac"),
    DEF(NetworkMetric, ConnectionStatus, L"status"),
    DEF(NetworkMetric, WifiSignal,       L"signal"),
    DEF(NetworkMetric, WifiRxRate,       L"rxrate"),
    DEF(NetworkMetric, WifiTxRate,       L"txrate"),
    DEF(NetworkMetric, Ssid,             nullptr),
    DEF(NetworkMetric, WifiRadioType,    L"radiotype"),
};

static const MetricDef PingDefs[] = {
    DEF(PingMetric, Rtt,             nullptr),
    DEF(PingMetric, PacketLoss,      L"loss"),
    DEF(PingMetric, MinRtt,          nullptr),
    DEF(PingMetric, MaxRtt,          nullptr),
    DEF(PingMetric, AvgRtt,          nullptr),
    DEF(PingMetric, Jitter,          nullptr),
    DEF(PingMetric, Ttl,             nullptr),
    DEF(PingMetric, PacketsSent,     L"sent"),
    DEF(PingMetric, PacketsReceived, L"received"),
    DEF(PingMetric, ResolvedIp,      L"ip"),
    DEF(PingMetric, Status,          nullptr),
};

static const MetricDef StorageDefs[] = {
    DEF(StorageMetric, ReadBytes,    nullptr),
    DEF(StorageMetric, WriteBytes,   nullptr),
    DEF(StorageMetric, ReadSpeed,    nullptr),
    DEF(StorageMetric, WriteSpeed,   nullptr),
    DEF(StorageMetric, UsedSpace,    L"used"),
    DEF(StorageMetric, FreeSpace,    L"free"),
    DEF(StorageMetric, TotalSpace,   L"total"),
    DEF(StorageMetric, QueueLength,  L"queue"),
    DEF(StorageMetric, BusyPercent,  L"busy"),
    DEF(StorageMetric, ReadsPerSec,  L"readiops"),
    DEF(StorageMetric, WritesPerSec, L"writeiops"),
    DEF(StorageMetric, Temperature,  L"temp"),
    DEF(StorageMetric, VolumeLabel,  L"label"),
    DEF(StorageMetric, FileSystem,   L"fs"),
    DEF(StorageMetric, DriveType,    L"type"),
};

static const MetricDef BatteryDefs[] = {
    DEF(BatteryMetric, ChargeLevel,        L"charge"),
    DEF(BatteryMetric, Charging,           nullptr),
    DEF(BatteryMetric, AcOnline,           nullptr),
    DEF(BatteryMetric, TimeToEmpty,        L"timeleft"),
    DEF(BatteryMetric, Rate,               nullptr),
    DEF(BatteryMetric, Voltage,            nullptr),
    DEF(BatteryMetric, RemainingCapacity,  L"remaining"),
    DEF(BatteryMetric, FullChargeCapacity, L"fullcapacity"),
    DEF(BatteryMetric, DesignCapacity,     L"design"),
    DEF(BatteryMetric, WearLevel,          L"wear"),
    DEF(BatteryMetric, CycleCount,         L"cycles"),
    DEF(BatteryMetric, Status,             nullptr),
    DEF(BatteryMetric, Chemistry,          nullptr),
    DEF(BatteryMetric, Manufacturer,       L"mfr"),
    DEF(BatteryMetric, SerialNumber,       L"serial"),
    DEF(BatteryMetric, DeviceName,         L"name"),
};

static const MetricDef SystemDefs[] = {
    DEF(SystemMetric, Uptime,       nullptr),
    DEF(SystemMetric, ProcessCount, L"processes"),
    DEF(SystemMetric, ThreadCount,  L"threads"),
    DEF(SystemMetric, HandleCount,  L"handles"),
    DEF(SystemMetric, OsName,       nullptr),
    DEF(SystemMetric, OsVersion,    nullptr),
    DEF(SystemMetric, OsBuild,      nullptr),
    DEF(SystemMetric, Hostname,     nullptr),
    DEF(SystemMetric, UserName,     L"user"),
    DEF(SystemMetric, BootTime,     nullptr),
};

static const MetricDef MotherboardDefs[] = {
    DEF(MotherboardMetric, Manufacturer,       L"mfr"),
    DEF(MotherboardMetric, Product,            L"model"),
    DEF(MotherboardMetric, SerialNumber,       L"serial"),
    DEF(MotherboardMetric, BiosVersion,        L"bios"),
    DEF(MotherboardMetric, BiosDate,           nullptr),
    DEF(MotherboardMetric, SystemManufacturer, L"sysmfr"),
    DEF(MotherboardMetric, SystemProduct,      L"sysmodel"),
};

static const MetricDef DisplayDefs[] = {
    DEF(DisplayMetric, Width,        nullptr),
    DEF(DisplayMetric, Height,       nullptr),
    DEF(DisplayMetric, RefreshRate,  L"refresh"),
    DEF(DisplayMetric, BitsPerPixel, L"bpp"),
    DEF(DisplayMetric, Primary,      nullptr),
    DEF(DisplayMetric, Count,        nullptr),
    DEF(DisplayMetric, Name,         nullptr),
    DEF(DisplayMetric, DeviceName,   L"device"),
    DEF(DisplayMetric, Resolution,   L"res"),
};

#undef DEF
#undef NHM_WIDEN
#undef NHM_WIDEN2

struct MetricTable
{
    const MetricDef* defs  = nullptr;
    size_t           count = 0;
};

template <size_t N>
static MetricTable Table(const MetricDef (&defs)[N]) { return { defs, N }; }

static MetricTable TableFor(Category category)
{
    switch (category)
    {
    case Category::GPU:          return Table(GPUDefs);
    case Category::CPU:          return Table(CPUDefs);
    case Category::Memory:       return Table(MemoryDefs);
    case Category::Network:      return Table(NetworkDefs);
    case Category::Ping:         return Table(PingDefs);
    case Category::Storage:      return Table(StorageDefs);
    case Category::Battery:      return Table(BatteryDefs);
    case Category::System:       return Table(SystemDefs);
    case Category::Motherboard:  return Table(MotherboardDefs);
    case Category::Display:      return Table(DisplayDefs);
    default:          return {};
    }
}

// str arrives trimmed and lowercased from Normalize; the table carries the CamelCase
// name, so the comparison lowers the table side as it walks rather than building a
// second string to throw away.
static bool EqualsLowered(const std::wstring& lowered, const wchar_t* name)
{
    size_t i = 0;
    for (; name[i]; ++i)
    {
        if (i >= lowered.size() || lowered[i] != static_cast<wchar_t>(std::towlower(name[i])))
            return false;
    }

    return i == lowered.size();
}

std::wstring MetricName(Category category, uint32_t id)
{
    MetricTable table = TableFor(category);

    for (size_t i = 0; i < table.count; ++i)
    {
        if (table.defs[i].id == id)
            return table.defs[i].name;
    }

    return L"Metric[" + std::to_wstring(id) + L"]";
}

uint32_t ParseMetric(Category category, const std::wstring& input)
{
    if (category == Category::Unknown)
        return static_cast<uint32_t>(-1);

    MetricTable table = TableFor(category);
    if (!table.defs)
        return static_cast<uint32_t>(-1);

    std::wstring str = Normalize(input);

    // Device count is the same question in every category, so it is answered
    // centrally in HardwareCore rather than metric-by-metric in every module.
    // Display has carried its own DisplayMetric::Count since v2 and it means the
    // same thing ("total active monitors"), so routing the name here reads the
    // same number a Display skin read before.
    if (str == L"count" || str == L"devicecount")
        return METRIC_DEVICE_COUNT;

    // Same for every category, and answered from the module's own state rather than from a
    // snapshot. "provider" is the short form; plain "status" stays with Battery and Ping.
    if (str == L"providerstatus" || str == L"provider")
        return METRIC_PROVIDER_STATUS;

    for (size_t i = 0; i < table.count; ++i)
    {
        const MetricDef& def = table.defs[i];

        if (EqualsLowered(str, def.name) || (def.alias && str == def.alias))
            return def.id;
    }

    // Unrecognised: the category's Unknown sentinel, which is exactly what
    // MetricCount() reports. A section variable logs it once (see Plugin.cpp).
    return MetricCount(category);
}
