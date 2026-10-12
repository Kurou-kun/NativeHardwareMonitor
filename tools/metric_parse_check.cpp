// Pins MetricParser to the behaviour of the if-chains it replaced: every canonical name,

// every alias and every reserved spelling maps to the id the old code returned, and every

// id prints the name a Debug=2 dump printed before.

//

// Generated from the pre-table source. Build and run (no framework, no project entry):

//   cl /EHsc /std:c++17 /I source tools\metric_parse_check.cpp source\Types\MetricParser.cpp



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

#include <cassert>

#include <cstdio>



int main()

{

    int checked = 0;



    // --- GPU

    assert(ParseMetric(Category::GPU, L"usage") == static_cast<uint32_t>(GpuMetric::Usage));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Usage)) == L"Usage");

    checked += 2;

    assert(ParseMetric(Category::GPU, L"coreclock") == static_cast<uint32_t>(GpuMetric::CoreClock));

    assert(ParseMetric(Category::GPU, L"clock") == static_cast<uint32_t>(GpuMetric::CoreClock));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::CoreClock)) == L"CoreClock");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"effectiveclock") == static_cast<uint32_t>(GpuMetric::EffectiveClock));

    assert(ParseMetric(Category::GPU, L"effclock") == static_cast<uint32_t>(GpuMetric::EffectiveClock));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::EffectiveClock)) == L"EffectiveClock");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"memoryclock") == static_cast<uint32_t>(GpuMetric::MemoryClock));

    assert(ParseMetric(Category::GPU, L"memclock") == static_cast<uint32_t>(GpuMetric::MemoryClock));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::MemoryClock)) == L"MemoryClock");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"temperature") == static_cast<uint32_t>(GpuMetric::Temperature));

    assert(ParseMetric(Category::GPU, L"temp") == static_cast<uint32_t>(GpuMetric::Temperature));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Temperature)) == L"Temperature");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"hotspottemperature") == static_cast<uint32_t>(GpuMetric::HotspotTemperature));

    assert(ParseMetric(Category::GPU, L"hotspot") == static_cast<uint32_t>(GpuMetric::HotspotTemperature));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::HotspotTemperature)) == L"HotspotTemperature");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"memorytemperature") == static_cast<uint32_t>(GpuMetric::MemoryTemperature));

    assert(ParseMetric(Category::GPU, L"memtemp") == static_cast<uint32_t>(GpuMetric::MemoryTemperature));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::MemoryTemperature)) == L"MemoryTemperature");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"vramused") == static_cast<uint32_t>(GpuMetric::VramUsed));

    assert(ParseMetric(Category::GPU, L"vram") == static_cast<uint32_t>(GpuMetric::VramUsed));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::VramUsed)) == L"VramUsed");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"vramtotal") == static_cast<uint32_t>(GpuMetric::VramTotal));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::VramTotal)) == L"VramTotal");

    checked += 2;

    assert(ParseMetric(Category::GPU, L"fanspeed") == static_cast<uint32_t>(GpuMetric::FanSpeed));

    assert(ParseMetric(Category::GPU, L"fan") == static_cast<uint32_t>(GpuMetric::FanSpeed));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::FanSpeed)) == L"FanSpeed");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"fanspeedrpm") == static_cast<uint32_t>(GpuMetric::FanSpeedRPM));

    assert(ParseMetric(Category::GPU, L"fanrpm") == static_cast<uint32_t>(GpuMetric::FanSpeedRPM));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::FanSpeedRPM)) == L"FanSpeedRPM");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"power") == static_cast<uint32_t>(GpuMetric::Power));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Power)) == L"Power");

    checked += 2;

    assert(ParseMetric(Category::GPU, L"powerlimit") == static_cast<uint32_t>(GpuMetric::PowerLimit));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PowerLimit)) == L"PowerLimit");

    checked += 2;

    assert(ParseMetric(Category::GPU, L"voltage") == static_cast<uint32_t>(GpuMetric::Voltage));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Voltage)) == L"Voltage");

    checked += 2;

    assert(ParseMetric(Category::GPU, L"intaketemperature") == static_cast<uint32_t>(GpuMetric::IntakeTemperature));

    assert(ParseMetric(Category::GPU, L"intaketemp") == static_cast<uint32_t>(GpuMetric::IntakeTemperature));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::IntakeTemperature)) == L"IntakeTemperature");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"totalboardpower") == static_cast<uint32_t>(GpuMetric::TotalBoardPower));

    assert(ParseMetric(Category::GPU, L"tbp") == static_cast<uint32_t>(GpuMetric::TotalBoardPower));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::TotalBoardPower)) == L"TotalBoardPower");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"name") == static_cast<uint32_t>(GpuMetric::Name));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Name)) == L"Name");

    checked += 2;

    assert(ParseMetric(Category::GPU, L"driverversion") == static_cast<uint32_t>(GpuMetric::DriverVersion));

    assert(ParseMetric(Category::GPU, L"driver") == static_cast<uint32_t>(GpuMetric::DriverVersion));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::DriverVersion)) == L"DriverVersion");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"vbiosversion") == static_cast<uint32_t>(GpuMetric::VbiosVersion));

    assert(ParseMetric(Category::GPU, L"vbios") == static_cast<uint32_t>(GpuMetric::VbiosVersion));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::VbiosVersion)) == L"VbiosVersion");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"pcideviceid") == static_cast<uint32_t>(GpuMetric::PciDeviceId));

    assert(ParseMetric(Category::GPU, L"pciid") == static_cast<uint32_t>(GpuMetric::PciDeviceId));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PciDeviceId)) == L"PciDeviceId");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"maxcoreclock") == static_cast<uint32_t>(GpuMetric::MaxCoreClock));

    assert(ParseMetric(Category::GPU, L"maxclock") == static_cast<uint32_t>(GpuMetric::MaxCoreClock));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::MaxCoreClock)) == L"MaxCoreClock");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"pcielinkgen") == static_cast<uint32_t>(GpuMetric::PcieLinkGen));

    assert(ParseMetric(Category::GPU, L"pciegen") == static_cast<uint32_t>(GpuMetric::PcieLinkGen));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PcieLinkGen)) == L"PcieLinkGen");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"pcielinkwidth") == static_cast<uint32_t>(GpuMetric::PcieLinkWidth));

    assert(ParseMetric(Category::GPU, L"pciewidth") == static_cast<uint32_t>(GpuMetric::PcieLinkWidth));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PcieLinkWidth)) == L"PcieLinkWidth");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"encoderusage") == static_cast<uint32_t>(GpuMetric::EncoderUsage));

    assert(ParseMetric(Category::GPU, L"encoder") == static_cast<uint32_t>(GpuMetric::EncoderUsage));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::EncoderUsage)) == L"EncoderUsage");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"decoderusage") == static_cast<uint32_t>(GpuMetric::DecoderUsage));

    assert(ParseMetric(Category::GPU, L"decoder") == static_cast<uint32_t>(GpuMetric::DecoderUsage));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::DecoderUsage)) == L"DecoderUsage");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"perfstate") == static_cast<uint32_t>(GpuMetric::PerfState));

    assert(ParseMetric(Category::GPU, L"pstate") == static_cast<uint32_t>(GpuMetric::PerfState));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PerfState)) == L"PerfState");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"throttlereasons") == static_cast<uint32_t>(GpuMetric::ThrottleReasons));

    assert(ParseMetric(Category::GPU, L"throttle") == static_cast<uint32_t>(GpuMetric::ThrottleReasons));

    assert(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::ThrottleReasons)) == L"ThrottleReasons");

    checked += 3;

    assert(ParseMetric(Category::GPU, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::GPU, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::GPU, L"nonsense")    == MetricCount(Category::GPU));

    assert(ParseMetric(Category::GPU, L"  Usage ")    == ParseMetric(Category::GPU, L"usage"));  // trim + case

    checked += 4;



    // --- CPU

    assert(ParseMetric(Category::CPU, L"usage") == static_cast<uint32_t>(CpuMetric::Usage));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Usage)) == L"Usage");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"clock") == static_cast<uint32_t>(CpuMetric::Clock));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Clock)) == L"Clock");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"maxclock") == static_cast<uint32_t>(CpuMetric::MaxClock));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::MaxClock)) == L"MaxClock");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"voltage") == static_cast<uint32_t>(CpuMetric::Voltage));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Voltage)) == L"Voltage");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"name") == static_cast<uint32_t>(CpuMetric::Name));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Name)) == L"Name");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"vendor") == static_cast<uint32_t>(CpuMetric::Vendor));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Vendor)) == L"Vendor");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"identifier") == static_cast<uint32_t>(CpuMetric::Identifier));

    assert(ParseMetric(Category::CPU, L"id") == static_cast<uint32_t>(CpuMetric::Identifier));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Identifier)) == L"Identifier");

    checked += 3;

    assert(ParseMetric(Category::CPU, L"microcodeversion") == static_cast<uint32_t>(CpuMetric::MicrocodeVersion));

    assert(ParseMetric(Category::CPU, L"microcode") == static_cast<uint32_t>(CpuMetric::MicrocodeVersion));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::MicrocodeVersion)) == L"MicrocodeVersion");

    checked += 3;

    assert(ParseMetric(Category::CPU, L"corecount") == static_cast<uint32_t>(CpuMetric::CoreCount));

    assert(ParseMetric(Category::CPU, L"cores") == static_cast<uint32_t>(CpuMetric::CoreCount));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::CoreCount)) == L"CoreCount");

    checked += 3;

    assert(ParseMetric(Category::CPU, L"threadcount") == static_cast<uint32_t>(CpuMetric::ThreadCount));

    assert(ParseMetric(Category::CPU, L"threads") == static_cast<uint32_t>(CpuMetric::ThreadCount));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::ThreadCount)) == L"ThreadCount");

    checked += 3;

    assert(ParseMetric(Category::CPU, L"cachel1") == static_cast<uint32_t>(CpuMetric::CacheL1));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::CacheL1)) == L"CacheL1");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"cachel2") == static_cast<uint32_t>(CpuMetric::CacheL2));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::CacheL2)) == L"CacheL2");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"cachel3") == static_cast<uint32_t>(CpuMetric::CacheL3));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::CacheL3)) == L"CacheL3");

    checked += 2;

    assert(ParseMetric(Category::CPU, L"architecture") == static_cast<uint32_t>(CpuMetric::Architecture));

    assert(ParseMetric(Category::CPU, L"arch") == static_cast<uint32_t>(CpuMetric::Architecture));

    assert(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Architecture)) == L"Architecture");

    checked += 3;

    assert(ParseMetric(Category::CPU, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::CPU, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::CPU, L"nonsense")    == MetricCount(Category::CPU));

    assert(ParseMetric(Category::CPU, L"  Usage ")    == ParseMetric(Category::CPU, L"usage"));  // trim + case

    checked += 4;



    // --- Memory

    assert(ParseMetric(Category::Memory, L"used") == static_cast<uint32_t>(MemoryMetric::Used));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Used)) == L"Used");

    checked += 2;

    assert(ParseMetric(Category::Memory, L"free") == static_cast<uint32_t>(MemoryMetric::Free));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Free)) == L"Free");

    checked += 2;

    assert(ParseMetric(Category::Memory, L"total") == static_cast<uint32_t>(MemoryMetric::Total));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Total)) == L"Total");

    checked += 2;

    assert(ParseMetric(Category::Memory, L"usedpercent") == static_cast<uint32_t>(MemoryMetric::UsedPercent));

    assert(ParseMetric(Category::Memory, L"usedpct") == static_cast<uint32_t>(MemoryMetric::UsedPercent));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::UsedPercent)) == L"UsedPercent");

    checked += 3;

    assert(ParseMetric(Category::Memory, L"cached") == static_cast<uint32_t>(MemoryMetric::Cached));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Cached)) == L"Cached");

    checked += 2;

    assert(ParseMetric(Category::Memory, L"speed") == static_cast<uint32_t>(MemoryMetric::Speed));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Speed)) == L"Speed");

    checked += 2;

    assert(ParseMetric(Category::Memory, L"memorytype") == static_cast<uint32_t>(MemoryMetric::MemoryType));

    assert(ParseMetric(Category::Memory, L"type") == static_cast<uint32_t>(MemoryMetric::MemoryType));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::MemoryType)) == L"MemoryType");

    checked += 3;

    assert(ParseMetric(Category::Memory, L"manufacturer") == static_cast<uint32_t>(MemoryMetric::Manufacturer));

    assert(ParseMetric(Category::Memory, L"mfr") == static_cast<uint32_t>(MemoryMetric::Manufacturer));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Manufacturer)) == L"Manufacturer");

    checked += 3;

    assert(ParseMetric(Category::Memory, L"partnumber") == static_cast<uint32_t>(MemoryMetric::PartNumber));

    assert(ParseMetric(Category::Memory, L"part") == static_cast<uint32_t>(MemoryMetric::PartNumber));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::PartNumber)) == L"PartNumber");

    checked += 3;

    assert(ParseMetric(Category::Memory, L"modulecount") == static_cast<uint32_t>(MemoryMetric::ModuleCount));

    assert(ParseMetric(Category::Memory, L"modules") == static_cast<uint32_t>(MemoryMetric::ModuleCount));

    assert(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::ModuleCount)) == L"ModuleCount");

    checked += 3;

    assert(ParseMetric(Category::Memory, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Memory, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Memory, L"nonsense")    == MetricCount(Category::Memory));

    assert(ParseMetric(Category::Memory, L"  Usage ")    == ParseMetric(Category::Memory, L"usage"));  // trim + case

    checked += 4;



    // --- Network

    assert(ParseMetric(Category::Network, L"download") == static_cast<uint32_t>(NetworkMetric::Download));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Download)) == L"Download");

    checked += 2;

    assert(ParseMetric(Category::Network, L"upload") == static_cast<uint32_t>(NetworkMetric::Upload));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Upload)) == L"Upload");

    checked += 2;

    assert(ParseMetric(Category::Network, L"downloadtotal") == static_cast<uint32_t>(NetworkMetric::DownloadTotal));

    assert(ParseMetric(Category::Network, L"dltotal") == static_cast<uint32_t>(NetworkMetric::DownloadTotal));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::DownloadTotal)) == L"DownloadTotal");

    checked += 3;

    assert(ParseMetric(Category::Network, L"uploadtotal") == static_cast<uint32_t>(NetworkMetric::UploadTotal));

    assert(ParseMetric(Category::Network, L"ultotal") == static_cast<uint32_t>(NetworkMetric::UploadTotal));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::UploadTotal)) == L"UploadTotal");

    checked += 3;

    assert(ParseMetric(Category::Network, L"speed") == static_cast<uint32_t>(NetworkMetric::Speed));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Speed)) == L"Speed");

    checked += 2;

    assert(ParseMetric(Category::Network, L"receivelinkspeed") == static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed));

    assert(ParseMetric(Category::Network, L"linkspeed") == static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed)) == L"ReceiveLinkSpeed");

    checked += 3;

    assert(ParseMetric(Category::Network, L"packetsreceived") == static_cast<uint32_t>(NetworkMetric::PacketsReceived));

    assert(ParseMetric(Category::Network, L"rxpackets") == static_cast<uint32_t>(NetworkMetric::PacketsReceived));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::PacketsReceived)) == L"PacketsReceived");

    checked += 3;

    assert(ParseMetric(Category::Network, L"packetssent") == static_cast<uint32_t>(NetworkMetric::PacketsSent));

    assert(ParseMetric(Category::Network, L"txpackets") == static_cast<uint32_t>(NetworkMetric::PacketsSent));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::PacketsSent)) == L"PacketsSent");

    checked += 3;

    assert(ParseMetric(Category::Network, L"errorsreceived") == static_cast<uint32_t>(NetworkMetric::ErrorsReceived));

    assert(ParseMetric(Category::Network, L"rxerrors") == static_cast<uint32_t>(NetworkMetric::ErrorsReceived));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::ErrorsReceived)) == L"ErrorsReceived");

    checked += 3;

    assert(ParseMetric(Category::Network, L"errorssent") == static_cast<uint32_t>(NetworkMetric::ErrorsSent));

    assert(ParseMetric(Category::Network, L"txerrors") == static_cast<uint32_t>(NetworkMetric::ErrorsSent));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::ErrorsSent)) == L"ErrorsSent");

    checked += 3;

    assert(ParseMetric(Category::Network, L"discardsreceived") == static_cast<uint32_t>(NetworkMetric::DiscardsReceived));

    assert(ParseMetric(Category::Network, L"rxdiscards") == static_cast<uint32_t>(NetworkMetric::DiscardsReceived));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::DiscardsReceived)) == L"DiscardsReceived");

    checked += 3;

    assert(ParseMetric(Category::Network, L"discardssent") == static_cast<uint32_t>(NetworkMetric::DiscardsSent));

    assert(ParseMetric(Category::Network, L"txdiscards") == static_cast<uint32_t>(NetworkMetric::DiscardsSent));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::DiscardsSent)) == L"DiscardsSent");

    checked += 3;

    assert(ParseMetric(Category::Network, L"mtu") == static_cast<uint32_t>(NetworkMetric::Mtu));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Mtu)) == L"Mtu");

    checked += 2;

    assert(ParseMetric(Category::Network, L"alias") == static_cast<uint32_t>(NetworkMetric::Alias));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Alias)) == L"Alias");

    checked += 2;

    assert(ParseMetric(Category::Network, L"description") == static_cast<uint32_t>(NetworkMetric::Description));

    assert(ParseMetric(Category::Network, L"desc") == static_cast<uint32_t>(NetworkMetric::Description));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Description)) == L"Description");

    checked += 3;

    assert(ParseMetric(Category::Network, L"physicaladdress") == static_cast<uint32_t>(NetworkMetric::PhysicalAddress));

    assert(ParseMetric(Category::Network, L"mac") == static_cast<uint32_t>(NetworkMetric::PhysicalAddress));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::PhysicalAddress)) == L"PhysicalAddress");

    checked += 3;

    assert(ParseMetric(Category::Network, L"connectionstatus") == static_cast<uint32_t>(NetworkMetric::ConnectionStatus));

    assert(ParseMetric(Category::Network, L"status") == static_cast<uint32_t>(NetworkMetric::ConnectionStatus));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::ConnectionStatus)) == L"ConnectionStatus");

    checked += 3;

    assert(ParseMetric(Category::Network, L"wifisignal") == static_cast<uint32_t>(NetworkMetric::WifiSignal));

    assert(ParseMetric(Category::Network, L"signal") == static_cast<uint32_t>(NetworkMetric::WifiSignal));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::WifiSignal)) == L"WifiSignal");

    checked += 3;

    assert(ParseMetric(Category::Network, L"wifirxrate") == static_cast<uint32_t>(NetworkMetric::WifiRxRate));

    assert(ParseMetric(Category::Network, L"rxrate") == static_cast<uint32_t>(NetworkMetric::WifiRxRate));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::WifiRxRate)) == L"WifiRxRate");

    checked += 3;

    assert(ParseMetric(Category::Network, L"wifitxrate") == static_cast<uint32_t>(NetworkMetric::WifiTxRate));

    assert(ParseMetric(Category::Network, L"txrate") == static_cast<uint32_t>(NetworkMetric::WifiTxRate));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::WifiTxRate)) == L"WifiTxRate");

    checked += 3;

    assert(ParseMetric(Category::Network, L"ssid") == static_cast<uint32_t>(NetworkMetric::Ssid));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Ssid)) == L"Ssid");

    checked += 2;

    assert(ParseMetric(Category::Network, L"wifiradiotype") == static_cast<uint32_t>(NetworkMetric::WifiRadioType));

    assert(ParseMetric(Category::Network, L"radiotype") == static_cast<uint32_t>(NetworkMetric::WifiRadioType));

    assert(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::WifiRadioType)) == L"WifiRadioType");

    checked += 3;

    assert(ParseMetric(Category::Network, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Network, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Network, L"nonsense")    == MetricCount(Category::Network));

    assert(ParseMetric(Category::Network, L"  Usage ")    == ParseMetric(Category::Network, L"usage"));  // trim + case

    checked += 4;



    // --- Ping

    assert(ParseMetric(Category::Ping, L"rtt") == static_cast<uint32_t>(PingMetric::Rtt));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::Rtt)) == L"Rtt");

    checked += 2;

    assert(ParseMetric(Category::Ping, L"packetloss") == static_cast<uint32_t>(PingMetric::PacketLoss));

    assert(ParseMetric(Category::Ping, L"loss") == static_cast<uint32_t>(PingMetric::PacketLoss));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::PacketLoss)) == L"PacketLoss");

    checked += 3;

    assert(ParseMetric(Category::Ping, L"minrtt") == static_cast<uint32_t>(PingMetric::MinRtt));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::MinRtt)) == L"MinRtt");

    checked += 2;

    assert(ParseMetric(Category::Ping, L"maxrtt") == static_cast<uint32_t>(PingMetric::MaxRtt));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::MaxRtt)) == L"MaxRtt");

    checked += 2;

    assert(ParseMetric(Category::Ping, L"avgrtt") == static_cast<uint32_t>(PingMetric::AvgRtt));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::AvgRtt)) == L"AvgRtt");

    checked += 2;

    assert(ParseMetric(Category::Ping, L"jitter") == static_cast<uint32_t>(PingMetric::Jitter));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::Jitter)) == L"Jitter");

    checked += 2;

    assert(ParseMetric(Category::Ping, L"ttl") == static_cast<uint32_t>(PingMetric::Ttl));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::Ttl)) == L"Ttl");

    checked += 2;

    assert(ParseMetric(Category::Ping, L"packetssent") == static_cast<uint32_t>(PingMetric::PacketsSent));

    assert(ParseMetric(Category::Ping, L"sent") == static_cast<uint32_t>(PingMetric::PacketsSent));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::PacketsSent)) == L"PacketsSent");

    checked += 3;

    assert(ParseMetric(Category::Ping, L"packetsreceived") == static_cast<uint32_t>(PingMetric::PacketsReceived));

    assert(ParseMetric(Category::Ping, L"received") == static_cast<uint32_t>(PingMetric::PacketsReceived));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::PacketsReceived)) == L"PacketsReceived");

    checked += 3;

    assert(ParseMetric(Category::Ping, L"resolvedip") == static_cast<uint32_t>(PingMetric::ResolvedIp));

    assert(ParseMetric(Category::Ping, L"ip") == static_cast<uint32_t>(PingMetric::ResolvedIp));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::ResolvedIp)) == L"ResolvedIp");

    checked += 3;

    assert(ParseMetric(Category::Ping, L"status") == static_cast<uint32_t>(PingMetric::Status));

    assert(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::Status)) == L"Status");

    checked += 2;

    assert(ParseMetric(Category::Ping, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Ping, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Ping, L"nonsense")    == MetricCount(Category::Ping));

    assert(ParseMetric(Category::Ping, L"  Usage ")    == ParseMetric(Category::Ping, L"usage"));  // trim + case

    checked += 4;



    // --- Storage

    assert(ParseMetric(Category::Storage, L"readbytes") == static_cast<uint32_t>(StorageMetric::ReadBytes));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::ReadBytes)) == L"ReadBytes");

    checked += 2;

    assert(ParseMetric(Category::Storage, L"writebytes") == static_cast<uint32_t>(StorageMetric::WriteBytes));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::WriteBytes)) == L"WriteBytes");

    checked += 2;

    assert(ParseMetric(Category::Storage, L"readspeed") == static_cast<uint32_t>(StorageMetric::ReadSpeed));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::ReadSpeed)) == L"ReadSpeed");

    checked += 2;

    assert(ParseMetric(Category::Storage, L"writespeed") == static_cast<uint32_t>(StorageMetric::WriteSpeed));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::WriteSpeed)) == L"WriteSpeed");

    checked += 2;

    assert(ParseMetric(Category::Storage, L"usedspace") == static_cast<uint32_t>(StorageMetric::UsedSpace));

    assert(ParseMetric(Category::Storage, L"used") == static_cast<uint32_t>(StorageMetric::UsedSpace));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::UsedSpace)) == L"UsedSpace");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"freespace") == static_cast<uint32_t>(StorageMetric::FreeSpace));

    assert(ParseMetric(Category::Storage, L"free") == static_cast<uint32_t>(StorageMetric::FreeSpace));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::FreeSpace)) == L"FreeSpace");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"totalspace") == static_cast<uint32_t>(StorageMetric::TotalSpace));

    assert(ParseMetric(Category::Storage, L"total") == static_cast<uint32_t>(StorageMetric::TotalSpace));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::TotalSpace)) == L"TotalSpace");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"queuelength") == static_cast<uint32_t>(StorageMetric::QueueLength));

    assert(ParseMetric(Category::Storage, L"queue") == static_cast<uint32_t>(StorageMetric::QueueLength));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::QueueLength)) == L"QueueLength");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"busypercent") == static_cast<uint32_t>(StorageMetric::BusyPercent));

    assert(ParseMetric(Category::Storage, L"busy") == static_cast<uint32_t>(StorageMetric::BusyPercent));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::BusyPercent)) == L"BusyPercent");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"readspersec") == static_cast<uint32_t>(StorageMetric::ReadsPerSec));

    assert(ParseMetric(Category::Storage, L"readiops") == static_cast<uint32_t>(StorageMetric::ReadsPerSec));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::ReadsPerSec)) == L"ReadsPerSec");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"writespersec") == static_cast<uint32_t>(StorageMetric::WritesPerSec));

    assert(ParseMetric(Category::Storage, L"writeiops") == static_cast<uint32_t>(StorageMetric::WritesPerSec));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::WritesPerSec)) == L"WritesPerSec");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"temperature") == static_cast<uint32_t>(StorageMetric::Temperature));

    assert(ParseMetric(Category::Storage, L"temp") == static_cast<uint32_t>(StorageMetric::Temperature));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::Temperature)) == L"Temperature");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"volumelabel") == static_cast<uint32_t>(StorageMetric::VolumeLabel));

    assert(ParseMetric(Category::Storage, L"label") == static_cast<uint32_t>(StorageMetric::VolumeLabel));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::VolumeLabel)) == L"VolumeLabel");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"filesystem") == static_cast<uint32_t>(StorageMetric::FileSystem));

    assert(ParseMetric(Category::Storage, L"fs") == static_cast<uint32_t>(StorageMetric::FileSystem));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::FileSystem)) == L"FileSystem");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"drivetype") == static_cast<uint32_t>(StorageMetric::DriveType));

    assert(ParseMetric(Category::Storage, L"type") == static_cast<uint32_t>(StorageMetric::DriveType));

    assert(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::DriveType)) == L"DriveType");

    checked += 3;

    assert(ParseMetric(Category::Storage, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Storage, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Storage, L"nonsense")    == MetricCount(Category::Storage));

    assert(ParseMetric(Category::Storage, L"  Usage ")    == ParseMetric(Category::Storage, L"usage"));  // trim + case

    checked += 4;



    // --- Battery

    assert(ParseMetric(Category::Battery, L"chargelevel") == static_cast<uint32_t>(BatteryMetric::ChargeLevel));

    assert(ParseMetric(Category::Battery, L"charge") == static_cast<uint32_t>(BatteryMetric::ChargeLevel));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::ChargeLevel)) == L"ChargeLevel");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"charging") == static_cast<uint32_t>(BatteryMetric::Charging));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Charging)) == L"Charging");

    checked += 2;

    assert(ParseMetric(Category::Battery, L"aconline") == static_cast<uint32_t>(BatteryMetric::AcOnline));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::AcOnline)) == L"AcOnline");

    checked += 2;

    assert(ParseMetric(Category::Battery, L"timetoempty") == static_cast<uint32_t>(BatteryMetric::TimeToEmpty));

    assert(ParseMetric(Category::Battery, L"timeleft") == static_cast<uint32_t>(BatteryMetric::TimeToEmpty));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::TimeToEmpty)) == L"TimeToEmpty");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"rate") == static_cast<uint32_t>(BatteryMetric::Rate));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Rate)) == L"Rate");

    checked += 2;

    assert(ParseMetric(Category::Battery, L"voltage") == static_cast<uint32_t>(BatteryMetric::Voltage));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Voltage)) == L"Voltage");

    checked += 2;

    assert(ParseMetric(Category::Battery, L"remainingcapacity") == static_cast<uint32_t>(BatteryMetric::RemainingCapacity));

    assert(ParseMetric(Category::Battery, L"remaining") == static_cast<uint32_t>(BatteryMetric::RemainingCapacity));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::RemainingCapacity)) == L"RemainingCapacity");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"fullchargecapacity") == static_cast<uint32_t>(BatteryMetric::FullChargeCapacity));

    assert(ParseMetric(Category::Battery, L"fullcapacity") == static_cast<uint32_t>(BatteryMetric::FullChargeCapacity));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::FullChargeCapacity)) == L"FullChargeCapacity");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"designcapacity") == static_cast<uint32_t>(BatteryMetric::DesignCapacity));

    assert(ParseMetric(Category::Battery, L"design") == static_cast<uint32_t>(BatteryMetric::DesignCapacity));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::DesignCapacity)) == L"DesignCapacity");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"wearlevel") == static_cast<uint32_t>(BatteryMetric::WearLevel));

    assert(ParseMetric(Category::Battery, L"wear") == static_cast<uint32_t>(BatteryMetric::WearLevel));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::WearLevel)) == L"WearLevel");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"cyclecount") == static_cast<uint32_t>(BatteryMetric::CycleCount));

    assert(ParseMetric(Category::Battery, L"cycles") == static_cast<uint32_t>(BatteryMetric::CycleCount));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::CycleCount)) == L"CycleCount");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"status") == static_cast<uint32_t>(BatteryMetric::Status));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Status)) == L"Status");

    checked += 2;

    assert(ParseMetric(Category::Battery, L"chemistry") == static_cast<uint32_t>(BatteryMetric::Chemistry));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Chemistry)) == L"Chemistry");

    checked += 2;

    assert(ParseMetric(Category::Battery, L"manufacturer") == static_cast<uint32_t>(BatteryMetric::Manufacturer));

    assert(ParseMetric(Category::Battery, L"mfr") == static_cast<uint32_t>(BatteryMetric::Manufacturer));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Manufacturer)) == L"Manufacturer");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"serialnumber") == static_cast<uint32_t>(BatteryMetric::SerialNumber));

    assert(ParseMetric(Category::Battery, L"serial") == static_cast<uint32_t>(BatteryMetric::SerialNumber));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::SerialNumber)) == L"SerialNumber");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"devicename") == static_cast<uint32_t>(BatteryMetric::DeviceName));

    assert(ParseMetric(Category::Battery, L"name") == static_cast<uint32_t>(BatteryMetric::DeviceName));

    assert(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::DeviceName)) == L"DeviceName");

    checked += 3;

    assert(ParseMetric(Category::Battery, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Battery, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Battery, L"nonsense")    == MetricCount(Category::Battery));

    assert(ParseMetric(Category::Battery, L"  Usage ")    == ParseMetric(Category::Battery, L"usage"));  // trim + case

    checked += 4;



    // --- System

    assert(ParseMetric(Category::System, L"uptime") == static_cast<uint32_t>(SystemMetric::Uptime));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::Uptime)) == L"Uptime");

    checked += 2;

    assert(ParseMetric(Category::System, L"processcount") == static_cast<uint32_t>(SystemMetric::ProcessCount));

    assert(ParseMetric(Category::System, L"processes") == static_cast<uint32_t>(SystemMetric::ProcessCount));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::ProcessCount)) == L"ProcessCount");

    checked += 3;

    assert(ParseMetric(Category::System, L"threadcount") == static_cast<uint32_t>(SystemMetric::ThreadCount));

    assert(ParseMetric(Category::System, L"threads") == static_cast<uint32_t>(SystemMetric::ThreadCount));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::ThreadCount)) == L"ThreadCount");

    checked += 3;

    assert(ParseMetric(Category::System, L"handlecount") == static_cast<uint32_t>(SystemMetric::HandleCount));

    assert(ParseMetric(Category::System, L"handles") == static_cast<uint32_t>(SystemMetric::HandleCount));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::HandleCount)) == L"HandleCount");

    checked += 3;

    assert(ParseMetric(Category::System, L"osname") == static_cast<uint32_t>(SystemMetric::OsName));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::OsName)) == L"OsName");

    checked += 2;

    assert(ParseMetric(Category::System, L"osversion") == static_cast<uint32_t>(SystemMetric::OsVersion));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::OsVersion)) == L"OsVersion");

    checked += 2;

    assert(ParseMetric(Category::System, L"osbuild") == static_cast<uint32_t>(SystemMetric::OsBuild));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::OsBuild)) == L"OsBuild");

    checked += 2;

    assert(ParseMetric(Category::System, L"hostname") == static_cast<uint32_t>(SystemMetric::Hostname));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::Hostname)) == L"Hostname");

    checked += 2;

    assert(ParseMetric(Category::System, L"username") == static_cast<uint32_t>(SystemMetric::UserName));

    assert(ParseMetric(Category::System, L"user") == static_cast<uint32_t>(SystemMetric::UserName));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::UserName)) == L"UserName");

    checked += 3;

    assert(ParseMetric(Category::System, L"boottime") == static_cast<uint32_t>(SystemMetric::BootTime));

    assert(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::BootTime)) == L"BootTime");

    checked += 2;

    assert(ParseMetric(Category::System, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::System, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::System, L"nonsense")    == MetricCount(Category::System));

    assert(ParseMetric(Category::System, L"  Usage ")    == ParseMetric(Category::System, L"usage"));  // trim + case

    checked += 4;



    // --- Motherboard

    assert(ParseMetric(Category::Motherboard, L"manufacturer") == static_cast<uint32_t>(MotherboardMetric::Manufacturer));

    assert(ParseMetric(Category::Motherboard, L"mfr") == static_cast<uint32_t>(MotherboardMetric::Manufacturer));

    assert(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::Manufacturer)) == L"Manufacturer");

    checked += 3;

    assert(ParseMetric(Category::Motherboard, L"product") == static_cast<uint32_t>(MotherboardMetric::Product));

    assert(ParseMetric(Category::Motherboard, L"model") == static_cast<uint32_t>(MotherboardMetric::Product));

    assert(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::Product)) == L"Product");

    checked += 3;

    assert(ParseMetric(Category::Motherboard, L"serialnumber") == static_cast<uint32_t>(MotherboardMetric::SerialNumber));

    assert(ParseMetric(Category::Motherboard, L"serial") == static_cast<uint32_t>(MotherboardMetric::SerialNumber));

    assert(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::SerialNumber)) == L"SerialNumber");

    checked += 3;

    assert(ParseMetric(Category::Motherboard, L"biosversion") == static_cast<uint32_t>(MotherboardMetric::BiosVersion));

    assert(ParseMetric(Category::Motherboard, L"bios") == static_cast<uint32_t>(MotherboardMetric::BiosVersion));

    assert(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::BiosVersion)) == L"BiosVersion");

    checked += 3;

    assert(ParseMetric(Category::Motherboard, L"biosdate") == static_cast<uint32_t>(MotherboardMetric::BiosDate));

    assert(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::BiosDate)) == L"BiosDate");

    checked += 2;

    assert(ParseMetric(Category::Motherboard, L"systemmanufacturer") == static_cast<uint32_t>(MotherboardMetric::SystemManufacturer));

    assert(ParseMetric(Category::Motherboard, L"sysmfr") == static_cast<uint32_t>(MotherboardMetric::SystemManufacturer));

    assert(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::SystemManufacturer)) == L"SystemManufacturer");

    checked += 3;

    assert(ParseMetric(Category::Motherboard, L"systemproduct") == static_cast<uint32_t>(MotherboardMetric::SystemProduct));

    assert(ParseMetric(Category::Motherboard, L"sysmodel") == static_cast<uint32_t>(MotherboardMetric::SystemProduct));

    assert(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::SystemProduct)) == L"SystemProduct");

    checked += 3;

    assert(ParseMetric(Category::Motherboard, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Motherboard, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Motherboard, L"nonsense")    == MetricCount(Category::Motherboard));

    assert(ParseMetric(Category::Motherboard, L"  Usage ")    == ParseMetric(Category::Motherboard, L"usage"));  // trim + case

    checked += 4;



    // --- Display

    assert(ParseMetric(Category::Display, L"width") == static_cast<uint32_t>(DisplayMetric::Width));

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Width)) == L"Width");

    checked += 2;

    assert(ParseMetric(Category::Display, L"height") == static_cast<uint32_t>(DisplayMetric::Height));

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Height)) == L"Height");

    checked += 2;

    assert(ParseMetric(Category::Display, L"refreshrate") == static_cast<uint32_t>(DisplayMetric::RefreshRate));

    assert(ParseMetric(Category::Display, L"refresh") == static_cast<uint32_t>(DisplayMetric::RefreshRate));

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::RefreshRate)) == L"RefreshRate");

    checked += 3;

    assert(ParseMetric(Category::Display, L"bitsperpixel") == static_cast<uint32_t>(DisplayMetric::BitsPerPixel));

    assert(ParseMetric(Category::Display, L"bpp") == static_cast<uint32_t>(DisplayMetric::BitsPerPixel));

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::BitsPerPixel)) == L"BitsPerPixel");

    checked += 3;

    assert(ParseMetric(Category::Display, L"primary") == static_cast<uint32_t>(DisplayMetric::Primary));

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Primary)) == L"Primary");

    checked += 2;

    // "count" is answered centrally before the table is consulted, in every category -
    // so DisplayMetric::Count is unreachable by name, exactly as it was unreachable past
    // the old if-chain. The name still has to print, which the next line checks.
    assert(ParseMetric(Category::Display, L"count") == METRIC_DEVICE_COUNT);

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Count)) == L"Count");

    checked += 2;

    assert(ParseMetric(Category::Display, L"name") == static_cast<uint32_t>(DisplayMetric::Name));

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Name)) == L"Name");

    checked += 2;

    assert(ParseMetric(Category::Display, L"devicename") == static_cast<uint32_t>(DisplayMetric::DeviceName));

    assert(ParseMetric(Category::Display, L"device") == static_cast<uint32_t>(DisplayMetric::DeviceName));

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::DeviceName)) == L"DeviceName");

    checked += 3;

    assert(ParseMetric(Category::Display, L"resolution") == static_cast<uint32_t>(DisplayMetric::Resolution));

    assert(ParseMetric(Category::Display, L"res") == static_cast<uint32_t>(DisplayMetric::Resolution));

    assert(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Resolution)) == L"Resolution");

    checked += 3;

    assert(ParseMetric(Category::Display, L"count")       == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Display, L"devicecount") == METRIC_DEVICE_COUNT);

    assert(ParseMetric(Category::Display, L"nonsense")    == MetricCount(Category::Display));

    assert(ParseMetric(Category::Display, L"  Usage ")    == ParseMetric(Category::Display, L"usage"));  // trim + case

    checked += 4;



    assert(ParseMetric(Category::Unknown, L"usage") == static_cast<uint32_t>(-1));



    printf("metric_parse_check: %d assertions passed\n", checked);
    return 0;

}

