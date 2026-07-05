#include "Types/MetricParser.h"

#include "Types/GpuMetric.h"
#include "Types/CpuMetric.h"
#include "Types/MemoryMetric.h"
#include "Types/NetworkMetric.h"
#include "Types/StorageMetric.h"
#include "Types/PingMetric.h"

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

Category ParseCategory(const std::wstring& input)
{
    std::wstring str = Normalize(input);

    if (str == L"gpu")     return Category::GPU;
    if (str == L"cpu")     return Category::CPU;
    if (str == L"memory")  return Category::Memory;
    if (str == L"network") return Category::Network;
    if (str == L"storage") return Category::Storage;
    if (str == L"ping")    return Category::Ping;

    return Category::Unknown;
}

uint32_t ParseMetric(Category category, const std::wstring& input)
{
    std::wstring str = Normalize(input);

    switch (category)
    {
    case Category::GPU:
        if (str == L"usage")              return static_cast<uint32_t>(GpuMetric::Usage);
        if (str == L"coreclock")          return static_cast<uint32_t>(GpuMetric::CoreClock);
        if (str == L"memoryclock")        return static_cast<uint32_t>(GpuMetric::MemoryClock);
        if (str == L"temperature")        return static_cast<uint32_t>(GpuMetric::Temperature);
        if (str == L"hotspottemperature") return static_cast<uint32_t>(GpuMetric::HotspotTemperature);
        if (str == L"memorytemperature")  return static_cast<uint32_t>(GpuMetric::MemoryTemperature);
        if (str == L"vramused")           return static_cast<uint32_t>(GpuMetric::VramUsed);
        if (str == L"vramtotal")          return static_cast<uint32_t>(GpuMetric::VramTotal);
        if (str == L"fanspeed")           return static_cast<uint32_t>(GpuMetric::FanSpeed);
        if (str == L"fanspeedrpm")        return static_cast<uint32_t>(GpuMetric::FanSpeedRPM);
        if (str == L"power")              return static_cast<uint32_t>(GpuMetric::Power);
        if (str == L"powerlimit")         return static_cast<uint32_t>(GpuMetric::PowerLimit);
        if (str == L"voltage")            return static_cast<uint32_t>(GpuMetric::Voltage);
        if (str == L"intaketemperature")  return static_cast<uint32_t>(GpuMetric::IntakeTemperature);
        if (str == L"totalboardpower")    return static_cast<uint32_t>(GpuMetric::TotalBoardPower);
        if (str == L"name")               return static_cast<uint32_t>(GpuMetric::Name);
        if (str == L"driverversion")      return static_cast<uint32_t>(GpuMetric::DriverVersion);
        if (str == L"vbiosversion")       return static_cast<uint32_t>(GpuMetric::VbiosVersion);
        if (str == L"pcideviceid")        return static_cast<uint32_t>(GpuMetric::PciDeviceId);
        return static_cast<uint32_t>(GpuMetric::Unknown);

    case Category::CPU:
        if (str == L"usage")    return static_cast<uint32_t>(CpuMetric::Usage);
        if (str == L"clock")    return static_cast<uint32_t>(CpuMetric::Clock);
        if (str == L"maxclock") return static_cast<uint32_t>(CpuMetric::MaxClock);
        if (str == L"voltage")  return static_cast<uint32_t>(CpuMetric::Voltage);
        if (str == L"name")             return static_cast<uint32_t>(CpuMetric::Name);
        if (str == L"vendor")           return static_cast<uint32_t>(CpuMetric::Vendor);
        if (str == L"identifier")       return static_cast<uint32_t>(CpuMetric::Identifier);
        if (str == L"microcodeversion") return static_cast<uint32_t>(CpuMetric::MicrocodeVersion);
        if (str == L"corecount")        return static_cast<uint32_t>(CpuMetric::CoreCount);
        if (str == L"threadcount")      return static_cast<uint32_t>(CpuMetric::ThreadCount);
        if (str == L"cachel1")          return static_cast<uint32_t>(CpuMetric::CacheL1);
        if (str == L"cachel2")          return static_cast<uint32_t>(CpuMetric::CacheL2);
        if (str == L"cachel3")          return static_cast<uint32_t>(CpuMetric::CacheL3);
        if (str == L"architecture")     return static_cast<uint32_t>(CpuMetric::Architecture);
        return static_cast<uint32_t>(CpuMetric::Unknown);

    case Category::Memory:
        if (str == L"used")        return static_cast<uint32_t>(MemoryMetric::Used);
        if (str == L"free")        return static_cast<uint32_t>(MemoryMetric::Free);
        if (str == L"total")       return static_cast<uint32_t>(MemoryMetric::Total);
        if (str == L"usedpercent") return static_cast<uint32_t>(MemoryMetric::UsedPercent);
        if (str == L"cached")      return static_cast<uint32_t>(MemoryMetric::Cached);
        if (str == L"speed")        return static_cast<uint32_t>(MemoryMetric::Speed);
        if (str == L"memorytype")   return static_cast<uint32_t>(MemoryMetric::MemoryType);
        if (str == L"manufacturer") return static_cast<uint32_t>(MemoryMetric::Manufacturer);
        if (str == L"partnumber")   return static_cast<uint32_t>(MemoryMetric::PartNumber);
        if (str == L"modulecount")  return static_cast<uint32_t>(MemoryMetric::ModuleCount);
        return static_cast<uint32_t>(MemoryMetric::Unknown);

    case Category::Network:
        if (str == L"download")      return static_cast<uint32_t>(NetworkMetric::Download);
        if (str == L"upload")        return static_cast<uint32_t>(NetworkMetric::Upload);
        if (str == L"downloadtotal")    return static_cast<uint32_t>(NetworkMetric::DownloadTotal);
        if (str == L"uploadtotal")      return static_cast<uint32_t>(NetworkMetric::UploadTotal);
        if (str == L"speed")            return static_cast<uint32_t>(NetworkMetric::Speed);
        if (str == L"receivelinkspeed") return static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed);
        if (str == L"packetsreceived")  return static_cast<uint32_t>(NetworkMetric::PacketsReceived);
        if (str == L"packetssent")      return static_cast<uint32_t>(NetworkMetric::PacketsSent);
        if (str == L"errorsreceived")   return static_cast<uint32_t>(NetworkMetric::ErrorsReceived);
        if (str == L"errorssent")       return static_cast<uint32_t>(NetworkMetric::ErrorsSent);
        if (str == L"discardsreceived") return static_cast<uint32_t>(NetworkMetric::DiscardsReceived);
        if (str == L"discardssent")     return static_cast<uint32_t>(NetworkMetric::DiscardsSent);
        if (str == L"mtu")              return static_cast<uint32_t>(NetworkMetric::Mtu);
        if (str == L"alias")            return static_cast<uint32_t>(NetworkMetric::Alias);
        if (str == L"description")      return static_cast<uint32_t>(NetworkMetric::Description);
        if (str == L"physicaladdress")  return static_cast<uint32_t>(NetworkMetric::PhysicalAddress);
        if (str == L"connectionstatus") return static_cast<uint32_t>(NetworkMetric::ConnectionStatus);
        return static_cast<uint32_t>(NetworkMetric::Unknown);

    case Category::Ping:
        if (str == L"rtt")        return static_cast<uint32_t>(PingMetric::Rtt);
        if (str == L"packetloss") return static_cast<uint32_t>(PingMetric::PacketLoss);
        return static_cast<uint32_t>(PingMetric::Unknown);

    case Category::Storage:
        if (str == L"readbytes")   return static_cast<uint32_t>(StorageMetric::ReadBytes);
        if (str == L"writebytes")  return static_cast<uint32_t>(StorageMetric::WriteBytes);
        if (str == L"readspeed")   return static_cast<uint32_t>(StorageMetric::ReadSpeed);
        if (str == L"writespeed")  return static_cast<uint32_t>(StorageMetric::WriteSpeed);
        if (str == L"usedspace")   return static_cast<uint32_t>(StorageMetric::UsedSpace);
        if (str == L"freespace")   return static_cast<uint32_t>(StorageMetric::FreeSpace);
        if (str == L"totalspace")  return static_cast<uint32_t>(StorageMetric::TotalSpace);
        if (str == L"queuelength")  return static_cast<uint32_t>(StorageMetric::QueueLength);
        if (str == L"busypercent") return static_cast<uint32_t>(StorageMetric::BusyPercent);
        if (str == L"readspersec") return static_cast<uint32_t>(StorageMetric::ReadsPerSec);
        if (str == L"writespersec") return static_cast<uint32_t>(StorageMetric::WritesPerSec);
        if (str == L"volumelabel") return static_cast<uint32_t>(StorageMetric::VolumeLabel);
        if (str == L"filesystem") return static_cast<uint32_t>(StorageMetric::FileSystem);
        if (str == L"drivetype")  return static_cast<uint32_t>(StorageMetric::DriveType);
        return static_cast<uint32_t>(StorageMetric::Unknown);

    default:
        return static_cast<uint32_t>(-1);
    }
}
