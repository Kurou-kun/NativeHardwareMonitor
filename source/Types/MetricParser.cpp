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

uint32_t ParseMetric(Category category, const std::wstring& input)
{
    std::wstring str = Normalize(input);

    switch (category)
    {
    case Category::GPU:
        if (str == L"usage")                                   return static_cast<uint32_t>(GpuMetric::Usage);
        if (str == L"coreclock" || str == L"clock")            return static_cast<uint32_t>(GpuMetric::CoreClock);
        if (str == L"memoryclock" || str == L"memclock")       return static_cast<uint32_t>(GpuMetric::MemoryClock);
        if (str == L"temperature" || str == L"temp")           return static_cast<uint32_t>(GpuMetric::Temperature);
        if (str == L"hotspottemperature" || str == L"hotspot") return static_cast<uint32_t>(GpuMetric::HotspotTemperature);
        if (str == L"memorytemperature" || str == L"memtemp")  return static_cast<uint32_t>(GpuMetric::MemoryTemperature);
        if (str == L"vramused" || str == L"vram")              return static_cast<uint32_t>(GpuMetric::VramUsed);
        if (str == L"vramtotal")                               return static_cast<uint32_t>(GpuMetric::VramTotal);
        if (str == L"fanspeed" || str == L"fan")               return static_cast<uint32_t>(GpuMetric::FanSpeed);
        if (str == L"fanspeedrpm" || str == L"fanrpm")         return static_cast<uint32_t>(GpuMetric::FanSpeedRPM);
        if (str == L"power")                                   return static_cast<uint32_t>(GpuMetric::Power);
        if (str == L"powerlimit")                              return static_cast<uint32_t>(GpuMetric::PowerLimit);
        if (str == L"voltage")                                 return static_cast<uint32_t>(GpuMetric::Voltage);
        if (str == L"intaketemperature" || str == L"intaketemp") return static_cast<uint32_t>(GpuMetric::IntakeTemperature);
        if (str == L"totalboardpower" || str == L"tbp")        return static_cast<uint32_t>(GpuMetric::TotalBoardPower);
        if (str == L"name")                                    return static_cast<uint32_t>(GpuMetric::Name);
        if (str == L"driverversion" || str == L"driver")       return static_cast<uint32_t>(GpuMetric::DriverVersion);
        if (str == L"vbiosversion" || str == L"vbios")         return static_cast<uint32_t>(GpuMetric::VbiosVersion);
        if (str == L"pcideviceid" || str == L"pciid")          return static_cast<uint32_t>(GpuMetric::PciDeviceId);
        if (str == L"maxcoreclock" || str == L"maxclock")      return static_cast<uint32_t>(GpuMetric::MaxCoreClock);
        if (str == L"pcielinkgen" || str == L"pciegen")        return static_cast<uint32_t>(GpuMetric::PcieLinkGen);
        if (str == L"pcielinkwidth" || str == L"pciewidth")    return static_cast<uint32_t>(GpuMetric::PcieLinkWidth);
        if (str == L"encoderusage" || str == L"encoder")       return static_cast<uint32_t>(GpuMetric::EncoderUsage);
        if (str == L"decoderusage" || str == L"decoder")       return static_cast<uint32_t>(GpuMetric::DecoderUsage);
        if (str == L"perfstate" || str == L"pstate")           return static_cast<uint32_t>(GpuMetric::PerfState);
        if (str == L"throttlereasons" || str == L"throttle")   return static_cast<uint32_t>(GpuMetric::ThrottleReasons);
        return static_cast<uint32_t>(GpuMetric::Unknown);

    case Category::CPU:
        if (str == L"usage")    return static_cast<uint32_t>(CpuMetric::Usage);
        if (str == L"clock")    return static_cast<uint32_t>(CpuMetric::Clock);
        if (str == L"maxclock") return static_cast<uint32_t>(CpuMetric::MaxClock);
        if (str == L"voltage")  return static_cast<uint32_t>(CpuMetric::Voltage);
        if (str == L"name")                                    return static_cast<uint32_t>(CpuMetric::Name);
        if (str == L"vendor")                                  return static_cast<uint32_t>(CpuMetric::Vendor);
        if (str == L"identifier" || str == L"id")              return static_cast<uint32_t>(CpuMetric::Identifier);
        if (str == L"microcodeversion" || str == L"microcode") return static_cast<uint32_t>(CpuMetric::MicrocodeVersion);
        if (str == L"corecount" || str == L"cores")            return static_cast<uint32_t>(CpuMetric::CoreCount);
        if (str == L"threadcount" || str == L"threads")        return static_cast<uint32_t>(CpuMetric::ThreadCount);
        if (str == L"cachel1")                                 return static_cast<uint32_t>(CpuMetric::CacheL1);
        if (str == L"cachel2")                                 return static_cast<uint32_t>(CpuMetric::CacheL2);
        if (str == L"cachel3")                                 return static_cast<uint32_t>(CpuMetric::CacheL3);
        if (str == L"architecture" || str == L"arch")          return static_cast<uint32_t>(CpuMetric::Architecture);
        return static_cast<uint32_t>(CpuMetric::Unknown);

    case Category::Memory:
        if (str == L"used")                             return static_cast<uint32_t>(MemoryMetric::Used);
        if (str == L"free")                             return static_cast<uint32_t>(MemoryMetric::Free);
        if (str == L"total")                            return static_cast<uint32_t>(MemoryMetric::Total);
        if (str == L"usedpercent" || str == L"usedpct") return static_cast<uint32_t>(MemoryMetric::UsedPercent);
        if (str == L"cached")                           return static_cast<uint32_t>(MemoryMetric::Cached);
        if (str == L"speed")                            return static_cast<uint32_t>(MemoryMetric::Speed);
        if (str == L"memorytype" || str == L"type")     return static_cast<uint32_t>(MemoryMetric::MemoryType);
        if (str == L"manufacturer" || str == L"mfr")    return static_cast<uint32_t>(MemoryMetric::Manufacturer);
        if (str == L"partnumber" || str == L"part")     return static_cast<uint32_t>(MemoryMetric::PartNumber);
        if (str == L"modulecount" || str == L"modules") return static_cast<uint32_t>(MemoryMetric::ModuleCount);
        return static_cast<uint32_t>(MemoryMetric::Unknown);

    case Category::Network:
        if (str == L"download")                                return static_cast<uint32_t>(NetworkMetric::Download);
        if (str == L"upload")                                  return static_cast<uint32_t>(NetworkMetric::Upload);
        if (str == L"downloadtotal" || str == L"dltotal")      return static_cast<uint32_t>(NetworkMetric::DownloadTotal);
        if (str == L"uploadtotal" || str == L"ultotal")        return static_cast<uint32_t>(NetworkMetric::UploadTotal);
        if (str == L"speed")                                   return static_cast<uint32_t>(NetworkMetric::Speed);
        if (str == L"receivelinkspeed" || str == L"linkspeed") return static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed);
        if (str == L"packetsreceived" || str == L"rxpackets")  return static_cast<uint32_t>(NetworkMetric::PacketsReceived);
        if (str == L"packetssent" || str == L"txpackets")      return static_cast<uint32_t>(NetworkMetric::PacketsSent);
        if (str == L"errorsreceived" || str == L"rxerrors")    return static_cast<uint32_t>(NetworkMetric::ErrorsReceived);
        if (str == L"errorssent" || str == L"txerrors")        return static_cast<uint32_t>(NetworkMetric::ErrorsSent);
        if (str == L"discardsreceived" || str == L"rxdiscards") return static_cast<uint32_t>(NetworkMetric::DiscardsReceived);
        if (str == L"discardssent" || str == L"txdiscards")    return static_cast<uint32_t>(NetworkMetric::DiscardsSent);
        if (str == L"mtu")                                     return static_cast<uint32_t>(NetworkMetric::Mtu);
        if (str == L"alias")                                   return static_cast<uint32_t>(NetworkMetric::Alias);
        if (str == L"description" || str == L"desc")           return static_cast<uint32_t>(NetworkMetric::Description);
        if (str == L"physicaladdress" || str == L"mac")        return static_cast<uint32_t>(NetworkMetric::PhysicalAddress);
        if (str == L"connectionstatus" || str == L"status")    return static_cast<uint32_t>(NetworkMetric::ConnectionStatus);
        if (str == L"wifisignal" || str == L"signal")          return static_cast<uint32_t>(NetworkMetric::WifiSignal);
        if (str == L"wifirxrate" || str == L"rxrate")          return static_cast<uint32_t>(NetworkMetric::WifiRxRate);
        if (str == L"wifitxrate" || str == L"txrate")          return static_cast<uint32_t>(NetworkMetric::WifiTxRate);
        if (str == L"ssid")                                    return static_cast<uint32_t>(NetworkMetric::Ssid);
        if (str == L"wifiradiotype" || str == L"radiotype")    return static_cast<uint32_t>(NetworkMetric::WifiRadioType);
        return static_cast<uint32_t>(NetworkMetric::Unknown);

    case Category::Ping:
        if (str == L"rtt")                                     return static_cast<uint32_t>(PingMetric::Rtt);
        if (str == L"packetloss" || str == L"loss")            return static_cast<uint32_t>(PingMetric::PacketLoss);
        if (str == L"minrtt")                                  return static_cast<uint32_t>(PingMetric::MinRtt);
        if (str == L"maxrtt")                                  return static_cast<uint32_t>(PingMetric::MaxRtt);
        if (str == L"avgrtt")                                  return static_cast<uint32_t>(PingMetric::AvgRtt);
        if (str == L"jitter")                                  return static_cast<uint32_t>(PingMetric::Jitter);
        if (str == L"ttl")                                     return static_cast<uint32_t>(PingMetric::Ttl);
        if (str == L"packetssent" || str == L"sent")           return static_cast<uint32_t>(PingMetric::PacketsSent);
        if (str == L"packetsreceived" || str == L"received")   return static_cast<uint32_t>(PingMetric::PacketsReceived);
        if (str == L"resolvedip" || str == L"ip")              return static_cast<uint32_t>(PingMetric::ResolvedIp);
        if (str == L"status")                                  return static_cast<uint32_t>(PingMetric::Status);
        return static_cast<uint32_t>(PingMetric::Unknown);

    case Category::Storage:
        if (str == L"readbytes")                          return static_cast<uint32_t>(StorageMetric::ReadBytes);
        if (str == L"writebytes")                         return static_cast<uint32_t>(StorageMetric::WriteBytes);
        if (str == L"readspeed")                          return static_cast<uint32_t>(StorageMetric::ReadSpeed);
        if (str == L"writespeed")                         return static_cast<uint32_t>(StorageMetric::WriteSpeed);
        if (str == L"usedspace" || str == L"used")        return static_cast<uint32_t>(StorageMetric::UsedSpace);
        if (str == L"freespace" || str == L"free")        return static_cast<uint32_t>(StorageMetric::FreeSpace);
        if (str == L"totalspace" || str == L"total")      return static_cast<uint32_t>(StorageMetric::TotalSpace);
        if (str == L"queuelength" || str == L"queue")     return static_cast<uint32_t>(StorageMetric::QueueLength);
        if (str == L"busypercent" || str == L"busy")      return static_cast<uint32_t>(StorageMetric::BusyPercent);
        if (str == L"readspersec" || str == L"readiops")  return static_cast<uint32_t>(StorageMetric::ReadsPerSec);
        if (str == L"writespersec" || str == L"writeiops") return static_cast<uint32_t>(StorageMetric::WritesPerSec);
        if (str == L"volumelabel" || str == L"label")     return static_cast<uint32_t>(StorageMetric::VolumeLabel);
        if (str == L"filesystem" || str == L"fs")         return static_cast<uint32_t>(StorageMetric::FileSystem);
        if (str == L"drivetype" || str == L"type")        return static_cast<uint32_t>(StorageMetric::DriveType);
        return static_cast<uint32_t>(StorageMetric::Unknown);

    case Category::Battery:
        if (str == L"chargelevel" || str == L"charge")              return static_cast<uint32_t>(BatteryMetric::ChargeLevel);
        if (str == L"charging")                                     return static_cast<uint32_t>(BatteryMetric::Charging);
        if (str == L"aconline")                                     return static_cast<uint32_t>(BatteryMetric::AcOnline);
        if (str == L"timetoempty" || str == L"timeleft")            return static_cast<uint32_t>(BatteryMetric::TimeToEmpty);
        if (str == L"rate")                                         return static_cast<uint32_t>(BatteryMetric::Rate);
        if (str == L"voltage")                                      return static_cast<uint32_t>(BatteryMetric::Voltage);
        if (str == L"remainingcapacity" || str == L"remaining")     return static_cast<uint32_t>(BatteryMetric::RemainingCapacity);
        if (str == L"fullchargecapacity" || str == L"fullcapacity") return static_cast<uint32_t>(BatteryMetric::FullChargeCapacity);
        if (str == L"designcapacity" || str == L"design")           return static_cast<uint32_t>(BatteryMetric::DesignCapacity);
        if (str == L"wearlevel" || str == L"wear")                  return static_cast<uint32_t>(BatteryMetric::WearLevel);
        if (str == L"cyclecount" || str == L"cycles")               return static_cast<uint32_t>(BatteryMetric::CycleCount);
        if (str == L"status")                                       return static_cast<uint32_t>(BatteryMetric::Status);
        if (str == L"chemistry")                                    return static_cast<uint32_t>(BatteryMetric::Chemistry);
        if (str == L"manufacturer" || str == L"mfr")                return static_cast<uint32_t>(BatteryMetric::Manufacturer);
        if (str == L"serialnumber" || str == L"serial")             return static_cast<uint32_t>(BatteryMetric::SerialNumber);
        if (str == L"devicename" || str == L"name")                 return static_cast<uint32_t>(BatteryMetric::DeviceName);
        return static_cast<uint32_t>(BatteryMetric::Unknown);

    case Category::System:
        if (str == L"uptime")                              return static_cast<uint32_t>(SystemMetric::Uptime);
        if (str == L"processcount" || str == L"processes") return static_cast<uint32_t>(SystemMetric::ProcessCount);
        if (str == L"threadcount" || str == L"threads")    return static_cast<uint32_t>(SystemMetric::ThreadCount);
        if (str == L"handlecount" || str == L"handles")    return static_cast<uint32_t>(SystemMetric::HandleCount);
        if (str == L"osname")                              return static_cast<uint32_t>(SystemMetric::OsName);
        if (str == L"osversion")                           return static_cast<uint32_t>(SystemMetric::OsVersion);
        if (str == L"osbuild")                             return static_cast<uint32_t>(SystemMetric::OsBuild);
        if (str == L"hostname")                            return static_cast<uint32_t>(SystemMetric::Hostname);
        if (str == L"username" || str == L"user")          return static_cast<uint32_t>(SystemMetric::UserName);
        if (str == L"boottime")                            return static_cast<uint32_t>(SystemMetric::BootTime);
        return static_cast<uint32_t>(SystemMetric::Unknown);

    case Category::Motherboard:
        if (str == L"manufacturer" || str == L"mfr")            return static_cast<uint32_t>(MotherboardMetric::Manufacturer);
        if (str == L"product" || str == L"model")               return static_cast<uint32_t>(MotherboardMetric::Product);
        if (str == L"serialnumber" || str == L"serial")         return static_cast<uint32_t>(MotherboardMetric::SerialNumber);
        if (str == L"biosversion" || str == L"bios")            return static_cast<uint32_t>(MotherboardMetric::BiosVersion);
        if (str == L"biosdate")                                 return static_cast<uint32_t>(MotherboardMetric::BiosDate);
        if (str == L"systemmanufacturer" || str == L"sysmfr")   return static_cast<uint32_t>(MotherboardMetric::SystemManufacturer);
        if (str == L"systemproduct" || str == L"sysmodel")      return static_cast<uint32_t>(MotherboardMetric::SystemProduct);
        return static_cast<uint32_t>(MotherboardMetric::Unknown);

    case Category::Display:
        if (str == L"width")                            return static_cast<uint32_t>(DisplayMetric::Width);
        if (str == L"height")                           return static_cast<uint32_t>(DisplayMetric::Height);
        if (str == L"refreshrate" || str == L"refresh") return static_cast<uint32_t>(DisplayMetric::RefreshRate);
        if (str == L"bitsperpixel" || str == L"bpp")    return static_cast<uint32_t>(DisplayMetric::BitsPerPixel);
        if (str == L"primary")                          return static_cast<uint32_t>(DisplayMetric::Primary);
        if (str == L"count")                            return static_cast<uint32_t>(DisplayMetric::Count);
        if (str == L"name")                             return static_cast<uint32_t>(DisplayMetric::Name);
        if (str == L"devicename" || str == L"device")   return static_cast<uint32_t>(DisplayMetric::DeviceName);
        if (str == L"resolution" || str == L"res")      return static_cast<uint32_t>(DisplayMetric::Resolution);
        return static_cast<uint32_t>(DisplayMetric::Unknown);

    default:
        return static_cast<uint32_t>(-1);
    }
}
