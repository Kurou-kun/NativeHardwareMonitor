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

// assert, plus a tally so a silent pass is distinguishable from a binary that ran
// nothing. NDEBUG would disarm assert, so the check is built without it.
#define CHECK(expr) do { assert(expr); ++checked; } while (0)



int main()

{

    int checked = 0;



    // --- GPU

    CHECK(ParseMetric(Category::GPU, L"usage") == static_cast<uint32_t>(GpuMetric::Usage));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Usage)) == L"Usage");


    CHECK(ParseMetric(Category::GPU, L"coreclock") == static_cast<uint32_t>(GpuMetric::CoreClock));

    CHECK(ParseMetric(Category::GPU, L"clock") == static_cast<uint32_t>(GpuMetric::CoreClock));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::CoreClock)) == L"CoreClock");


    CHECK(ParseMetric(Category::GPU, L"effectiveclock") == static_cast<uint32_t>(GpuMetric::EffectiveClock));

    CHECK(ParseMetric(Category::GPU, L"effclock") == static_cast<uint32_t>(GpuMetric::EffectiveClock));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::EffectiveClock)) == L"EffectiveClock");


    CHECK(ParseMetric(Category::GPU, L"memoryclock") == static_cast<uint32_t>(GpuMetric::MemoryClock));

    CHECK(ParseMetric(Category::GPU, L"memclock") == static_cast<uint32_t>(GpuMetric::MemoryClock));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::MemoryClock)) == L"MemoryClock");


    CHECK(ParseMetric(Category::GPU, L"temperature") == static_cast<uint32_t>(GpuMetric::Temperature));

    CHECK(ParseMetric(Category::GPU, L"temp") == static_cast<uint32_t>(GpuMetric::Temperature));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Temperature)) == L"Temperature");


    CHECK(ParseMetric(Category::GPU, L"hotspottemperature") == static_cast<uint32_t>(GpuMetric::HotspotTemperature));

    CHECK(ParseMetric(Category::GPU, L"hotspot") == static_cast<uint32_t>(GpuMetric::HotspotTemperature));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::HotspotTemperature)) == L"HotspotTemperature");


    CHECK(ParseMetric(Category::GPU, L"memorytemperature") == static_cast<uint32_t>(GpuMetric::MemoryTemperature));

    CHECK(ParseMetric(Category::GPU, L"memtemp") == static_cast<uint32_t>(GpuMetric::MemoryTemperature));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::MemoryTemperature)) == L"MemoryTemperature");


    CHECK(ParseMetric(Category::GPU, L"vramused") == static_cast<uint32_t>(GpuMetric::VramUsed));

    CHECK(ParseMetric(Category::GPU, L"vram") == static_cast<uint32_t>(GpuMetric::VramUsed));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::VramUsed)) == L"VramUsed");


    CHECK(ParseMetric(Category::GPU, L"vramtotal") == static_cast<uint32_t>(GpuMetric::VramTotal));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::VramTotal)) == L"VramTotal");


    CHECK(ParseMetric(Category::GPU, L"vramshared") == static_cast<uint32_t>(GpuMetric::VramShared));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::VramShared)) == L"VramShared");


    CHECK(ParseMetric(Category::GPU, L"fanspeed") == static_cast<uint32_t>(GpuMetric::FanSpeed));

    CHECK(ParseMetric(Category::GPU, L"fan") == static_cast<uint32_t>(GpuMetric::FanSpeed));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::FanSpeed)) == L"FanSpeed");


    CHECK(ParseMetric(Category::GPU, L"fanspeedrpm") == static_cast<uint32_t>(GpuMetric::FanSpeedRPM));

    CHECK(ParseMetric(Category::GPU, L"fanrpm") == static_cast<uint32_t>(GpuMetric::FanSpeedRPM));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::FanSpeedRPM)) == L"FanSpeedRPM");


    CHECK(ParseMetric(Category::GPU, L"power") == static_cast<uint32_t>(GpuMetric::Power));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Power)) == L"Power");


    CHECK(ParseMetric(Category::GPU, L"powerlimit") == static_cast<uint32_t>(GpuMetric::PowerLimit));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PowerLimit)) == L"PowerLimit");


    CHECK(ParseMetric(Category::GPU, L"voltage") == static_cast<uint32_t>(GpuMetric::Voltage));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Voltage)) == L"Voltage");


    CHECK(ParseMetric(Category::GPU, L"intaketemperature") == static_cast<uint32_t>(GpuMetric::IntakeTemperature));

    CHECK(ParseMetric(Category::GPU, L"intaketemp") == static_cast<uint32_t>(GpuMetric::IntakeTemperature));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::IntakeTemperature)) == L"IntakeTemperature");


    CHECK(ParseMetric(Category::GPU, L"totalboardpower") == static_cast<uint32_t>(GpuMetric::TotalBoardPower));

    CHECK(ParseMetric(Category::GPU, L"tbp") == static_cast<uint32_t>(GpuMetric::TotalBoardPower));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::TotalBoardPower)) == L"TotalBoardPower");


    CHECK(ParseMetric(Category::GPU, L"name") == static_cast<uint32_t>(GpuMetric::Name));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::Name)) == L"Name");


    CHECK(ParseMetric(Category::GPU, L"driverversion") == static_cast<uint32_t>(GpuMetric::DriverVersion));

    CHECK(ParseMetric(Category::GPU, L"driver") == static_cast<uint32_t>(GpuMetric::DriverVersion));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::DriverVersion)) == L"DriverVersion");


    CHECK(ParseMetric(Category::GPU, L"vbiosversion") == static_cast<uint32_t>(GpuMetric::VbiosVersion));

    CHECK(ParseMetric(Category::GPU, L"vbios") == static_cast<uint32_t>(GpuMetric::VbiosVersion));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::VbiosVersion)) == L"VbiosVersion");


    CHECK(ParseMetric(Category::GPU, L"pcideviceid") == static_cast<uint32_t>(GpuMetric::PciDeviceId));

    CHECK(ParseMetric(Category::GPU, L"pciid") == static_cast<uint32_t>(GpuMetric::PciDeviceId));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PciDeviceId)) == L"PciDeviceId");


    CHECK(ParseMetric(Category::GPU, L"maxcoreclock") == static_cast<uint32_t>(GpuMetric::MaxCoreClock));

    CHECK(ParseMetric(Category::GPU, L"maxclock") == static_cast<uint32_t>(GpuMetric::MaxCoreClock));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::MaxCoreClock)) == L"MaxCoreClock");


    CHECK(ParseMetric(Category::GPU, L"pcielinkgen") == static_cast<uint32_t>(GpuMetric::PcieLinkGen));

    CHECK(ParseMetric(Category::GPU, L"pciegen") == static_cast<uint32_t>(GpuMetric::PcieLinkGen));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PcieLinkGen)) == L"PcieLinkGen");


    CHECK(ParseMetric(Category::GPU, L"pcielinkwidth") == static_cast<uint32_t>(GpuMetric::PcieLinkWidth));

    CHECK(ParseMetric(Category::GPU, L"pciewidth") == static_cast<uint32_t>(GpuMetric::PcieLinkWidth));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PcieLinkWidth)) == L"PcieLinkWidth");


    CHECK(ParseMetric(Category::GPU, L"encoderusage") == static_cast<uint32_t>(GpuMetric::EncoderUsage));

    CHECK(ParseMetric(Category::GPU, L"encoder") == static_cast<uint32_t>(GpuMetric::EncoderUsage));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::EncoderUsage)) == L"EncoderUsage");


    CHECK(ParseMetric(Category::GPU, L"decoderusage") == static_cast<uint32_t>(GpuMetric::DecoderUsage));

    CHECK(ParseMetric(Category::GPU, L"decoder") == static_cast<uint32_t>(GpuMetric::DecoderUsage));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::DecoderUsage)) == L"DecoderUsage");


    CHECK(ParseMetric(Category::GPU, L"perfstate") == static_cast<uint32_t>(GpuMetric::PerfState));

    CHECK(ParseMetric(Category::GPU, L"pstate") == static_cast<uint32_t>(GpuMetric::PerfState));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::PerfState)) == L"PerfState");


    CHECK(ParseMetric(Category::GPU, L"throttlereasons") == static_cast<uint32_t>(GpuMetric::ThrottleReasons));

    CHECK(ParseMetric(Category::GPU, L"throttle") == static_cast<uint32_t>(GpuMetric::ThrottleReasons));

    CHECK(MetricName(Category::GPU, static_cast<uint32_t>(GpuMetric::ThrottleReasons)) == L"ThrottleReasons");


    CHECK(ParseMetric(Category::GPU, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::GPU, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::GPU, L"nonsense")    == MetricCount(Category::GPU));

    assert(ParseMetric(Category::GPU, L"  Usage ")    == ParseMetric(Category::GPU, L"usage"));  // trim + case




    // --- CPU

    CHECK(ParseMetric(Category::CPU, L"usage") == static_cast<uint32_t>(CpuMetric::Usage));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Usage)) == L"Usage");


    CHECK(ParseMetric(Category::CPU, L"clock") == static_cast<uint32_t>(CpuMetric::Clock));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Clock)) == L"Clock");


    CHECK(ParseMetric(Category::CPU, L"maxclock") == static_cast<uint32_t>(CpuMetric::MaxClock));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::MaxClock)) == L"MaxClock");


    CHECK(ParseMetric(Category::CPU, L"voltage") == static_cast<uint32_t>(CpuMetric::Voltage));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Voltage)) == L"Voltage");


    CHECK(ParseMetric(Category::CPU, L"name") == static_cast<uint32_t>(CpuMetric::Name));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Name)) == L"Name");


    CHECK(ParseMetric(Category::CPU, L"vendor") == static_cast<uint32_t>(CpuMetric::Vendor));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Vendor)) == L"Vendor");


    CHECK(ParseMetric(Category::CPU, L"identifier") == static_cast<uint32_t>(CpuMetric::Identifier));

    CHECK(ParseMetric(Category::CPU, L"id") == static_cast<uint32_t>(CpuMetric::Identifier));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Identifier)) == L"Identifier");


    CHECK(ParseMetric(Category::CPU, L"microcodeversion") == static_cast<uint32_t>(CpuMetric::MicrocodeVersion));

    CHECK(ParseMetric(Category::CPU, L"microcode") == static_cast<uint32_t>(CpuMetric::MicrocodeVersion));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::MicrocodeVersion)) == L"MicrocodeVersion");


    CHECK(ParseMetric(Category::CPU, L"corecount") == static_cast<uint32_t>(CpuMetric::CoreCount));

    CHECK(ParseMetric(Category::CPU, L"cores") == static_cast<uint32_t>(CpuMetric::CoreCount));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::CoreCount)) == L"CoreCount");


    CHECK(ParseMetric(Category::CPU, L"threadcount") == static_cast<uint32_t>(CpuMetric::ThreadCount));

    CHECK(ParseMetric(Category::CPU, L"threads") == static_cast<uint32_t>(CpuMetric::ThreadCount));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::ThreadCount)) == L"ThreadCount");


    CHECK(ParseMetric(Category::CPU, L"cachel1") == static_cast<uint32_t>(CpuMetric::CacheL1));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::CacheL1)) == L"CacheL1");


    CHECK(ParseMetric(Category::CPU, L"cachel2") == static_cast<uint32_t>(CpuMetric::CacheL2));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::CacheL2)) == L"CacheL2");


    CHECK(ParseMetric(Category::CPU, L"cachel3") == static_cast<uint32_t>(CpuMetric::CacheL3));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::CacheL3)) == L"CacheL3");


    CHECK(ParseMetric(Category::CPU, L"architecture") == static_cast<uint32_t>(CpuMetric::Architecture));

    CHECK(ParseMetric(Category::CPU, L"arch") == static_cast<uint32_t>(CpuMetric::Architecture));

    CHECK(MetricName(Category::CPU, static_cast<uint32_t>(CpuMetric::Architecture)) == L"Architecture");


    CHECK(ParseMetric(Category::CPU, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::CPU, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::CPU, L"nonsense")    == MetricCount(Category::CPU));

    assert(ParseMetric(Category::CPU, L"  Usage ")    == ParseMetric(Category::CPU, L"usage"));  // trim + case




    // --- Memory

    CHECK(ParseMetric(Category::Memory, L"used") == static_cast<uint32_t>(MemoryMetric::Used));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Used)) == L"Used");


    CHECK(ParseMetric(Category::Memory, L"free") == static_cast<uint32_t>(MemoryMetric::Free));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Free)) == L"Free");


    CHECK(ParseMetric(Category::Memory, L"total") == static_cast<uint32_t>(MemoryMetric::Total));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Total)) == L"Total");


    CHECK(ParseMetric(Category::Memory, L"usedpercent") == static_cast<uint32_t>(MemoryMetric::UsedPercent));

    CHECK(ParseMetric(Category::Memory, L"usedpct") == static_cast<uint32_t>(MemoryMetric::UsedPercent));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::UsedPercent)) == L"UsedPercent");


    CHECK(ParseMetric(Category::Memory, L"cached") == static_cast<uint32_t>(MemoryMetric::Cached));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Cached)) == L"Cached");


    CHECK(ParseMetric(Category::Memory, L"speed") == static_cast<uint32_t>(MemoryMetric::Speed));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Speed)) == L"Speed");


    CHECK(ParseMetric(Category::Memory, L"memorytype") == static_cast<uint32_t>(MemoryMetric::MemoryType));

    CHECK(ParseMetric(Category::Memory, L"type") == static_cast<uint32_t>(MemoryMetric::MemoryType));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::MemoryType)) == L"MemoryType");


    CHECK(ParseMetric(Category::Memory, L"manufacturer") == static_cast<uint32_t>(MemoryMetric::Manufacturer));

    CHECK(ParseMetric(Category::Memory, L"mfr") == static_cast<uint32_t>(MemoryMetric::Manufacturer));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::Manufacturer)) == L"Manufacturer");


    CHECK(ParseMetric(Category::Memory, L"partnumber") == static_cast<uint32_t>(MemoryMetric::PartNumber));

    CHECK(ParseMetric(Category::Memory, L"part") == static_cast<uint32_t>(MemoryMetric::PartNumber));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::PartNumber)) == L"PartNumber");


    CHECK(ParseMetric(Category::Memory, L"modulecount") == static_cast<uint32_t>(MemoryMetric::ModuleCount));

    CHECK(ParseMetric(Category::Memory, L"modules") == static_cast<uint32_t>(MemoryMetric::ModuleCount));

    CHECK(MetricName(Category::Memory, static_cast<uint32_t>(MemoryMetric::ModuleCount)) == L"ModuleCount");


    CHECK(ParseMetric(Category::Memory, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Memory, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Memory, L"nonsense")    == MetricCount(Category::Memory));

    assert(ParseMetric(Category::Memory, L"  Usage ")    == ParseMetric(Category::Memory, L"usage"));  // trim + case




    // --- Network

    CHECK(ParseMetric(Category::Network, L"download") == static_cast<uint32_t>(NetworkMetric::Download));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Download)) == L"Download");


    CHECK(ParseMetric(Category::Network, L"upload") == static_cast<uint32_t>(NetworkMetric::Upload));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Upload)) == L"Upload");


    CHECK(ParseMetric(Category::Network, L"downloadtotal") == static_cast<uint32_t>(NetworkMetric::DownloadTotal));

    CHECK(ParseMetric(Category::Network, L"dltotal") == static_cast<uint32_t>(NetworkMetric::DownloadTotal));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::DownloadTotal)) == L"DownloadTotal");


    CHECK(ParseMetric(Category::Network, L"uploadtotal") == static_cast<uint32_t>(NetworkMetric::UploadTotal));

    CHECK(ParseMetric(Category::Network, L"ultotal") == static_cast<uint32_t>(NetworkMetric::UploadTotal));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::UploadTotal)) == L"UploadTotal");


    CHECK(ParseMetric(Category::Network, L"speed") == static_cast<uint32_t>(NetworkMetric::Speed));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Speed)) == L"Speed");


    CHECK(ParseMetric(Category::Network, L"receivelinkspeed") == static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed));

    CHECK(ParseMetric(Category::Network, L"linkspeed") == static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::ReceiveLinkSpeed)) == L"ReceiveLinkSpeed");


    CHECK(ParseMetric(Category::Network, L"packetsreceived") == static_cast<uint32_t>(NetworkMetric::PacketsReceived));

    CHECK(ParseMetric(Category::Network, L"rxpackets") == static_cast<uint32_t>(NetworkMetric::PacketsReceived));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::PacketsReceived)) == L"PacketsReceived");


    CHECK(ParseMetric(Category::Network, L"packetssent") == static_cast<uint32_t>(NetworkMetric::PacketsSent));

    CHECK(ParseMetric(Category::Network, L"txpackets") == static_cast<uint32_t>(NetworkMetric::PacketsSent));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::PacketsSent)) == L"PacketsSent");


    CHECK(ParseMetric(Category::Network, L"errorsreceived") == static_cast<uint32_t>(NetworkMetric::ErrorsReceived));

    CHECK(ParseMetric(Category::Network, L"rxerrors") == static_cast<uint32_t>(NetworkMetric::ErrorsReceived));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::ErrorsReceived)) == L"ErrorsReceived");


    CHECK(ParseMetric(Category::Network, L"errorssent") == static_cast<uint32_t>(NetworkMetric::ErrorsSent));

    CHECK(ParseMetric(Category::Network, L"txerrors") == static_cast<uint32_t>(NetworkMetric::ErrorsSent));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::ErrorsSent)) == L"ErrorsSent");


    CHECK(ParseMetric(Category::Network, L"discardsreceived") == static_cast<uint32_t>(NetworkMetric::DiscardsReceived));

    CHECK(ParseMetric(Category::Network, L"rxdiscards") == static_cast<uint32_t>(NetworkMetric::DiscardsReceived));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::DiscardsReceived)) == L"DiscardsReceived");


    CHECK(ParseMetric(Category::Network, L"discardssent") == static_cast<uint32_t>(NetworkMetric::DiscardsSent));

    CHECK(ParseMetric(Category::Network, L"txdiscards") == static_cast<uint32_t>(NetworkMetric::DiscardsSent));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::DiscardsSent)) == L"DiscardsSent");


    CHECK(ParseMetric(Category::Network, L"mtu") == static_cast<uint32_t>(NetworkMetric::Mtu));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Mtu)) == L"Mtu");


    CHECK(ParseMetric(Category::Network, L"alias") == static_cast<uint32_t>(NetworkMetric::Alias));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Alias)) == L"Alias");


    CHECK(ParseMetric(Category::Network, L"description") == static_cast<uint32_t>(NetworkMetric::Description));

    CHECK(ParseMetric(Category::Network, L"desc") == static_cast<uint32_t>(NetworkMetric::Description));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Description)) == L"Description");


    CHECK(ParseMetric(Category::Network, L"physicaladdress") == static_cast<uint32_t>(NetworkMetric::PhysicalAddress));

    CHECK(ParseMetric(Category::Network, L"mac") == static_cast<uint32_t>(NetworkMetric::PhysicalAddress));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::PhysicalAddress)) == L"PhysicalAddress");


    CHECK(ParseMetric(Category::Network, L"connectionstatus") == static_cast<uint32_t>(NetworkMetric::ConnectionStatus));

    CHECK(ParseMetric(Category::Network, L"status") == static_cast<uint32_t>(NetworkMetric::ConnectionStatus));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::ConnectionStatus)) == L"ConnectionStatus");


    CHECK(ParseMetric(Category::Network, L"wifisignal") == static_cast<uint32_t>(NetworkMetric::WifiSignal));

    CHECK(ParseMetric(Category::Network, L"signal") == static_cast<uint32_t>(NetworkMetric::WifiSignal));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::WifiSignal)) == L"WifiSignal");


    CHECK(ParseMetric(Category::Network, L"wifirxrate") == static_cast<uint32_t>(NetworkMetric::WifiRxRate));

    CHECK(ParseMetric(Category::Network, L"rxrate") == static_cast<uint32_t>(NetworkMetric::WifiRxRate));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::WifiRxRate)) == L"WifiRxRate");


    CHECK(ParseMetric(Category::Network, L"wifitxrate") == static_cast<uint32_t>(NetworkMetric::WifiTxRate));

    CHECK(ParseMetric(Category::Network, L"txrate") == static_cast<uint32_t>(NetworkMetric::WifiTxRate));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::WifiTxRate)) == L"WifiTxRate");


    CHECK(ParseMetric(Category::Network, L"ssid") == static_cast<uint32_t>(NetworkMetric::Ssid));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::Ssid)) == L"Ssid");


    CHECK(ParseMetric(Category::Network, L"wifiradiotype") == static_cast<uint32_t>(NetworkMetric::WifiRadioType));

    CHECK(ParseMetric(Category::Network, L"radiotype") == static_cast<uint32_t>(NetworkMetric::WifiRadioType));

    CHECK(MetricName(Category::Network, static_cast<uint32_t>(NetworkMetric::WifiRadioType)) == L"WifiRadioType");


    CHECK(ParseMetric(Category::Network, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Network, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Network, L"nonsense")    == MetricCount(Category::Network));

    assert(ParseMetric(Category::Network, L"  Usage ")    == ParseMetric(Category::Network, L"usage"));  // trim + case




    // --- Ping

    CHECK(ParseMetric(Category::Ping, L"rtt") == static_cast<uint32_t>(PingMetric::Rtt));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::Rtt)) == L"Rtt");


    CHECK(ParseMetric(Category::Ping, L"packetloss") == static_cast<uint32_t>(PingMetric::PacketLoss));

    CHECK(ParseMetric(Category::Ping, L"loss") == static_cast<uint32_t>(PingMetric::PacketLoss));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::PacketLoss)) == L"PacketLoss");


    CHECK(ParseMetric(Category::Ping, L"minrtt") == static_cast<uint32_t>(PingMetric::MinRtt));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::MinRtt)) == L"MinRtt");


    CHECK(ParseMetric(Category::Ping, L"maxrtt") == static_cast<uint32_t>(PingMetric::MaxRtt));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::MaxRtt)) == L"MaxRtt");


    CHECK(ParseMetric(Category::Ping, L"avgrtt") == static_cast<uint32_t>(PingMetric::AvgRtt));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::AvgRtt)) == L"AvgRtt");


    CHECK(ParseMetric(Category::Ping, L"jitter") == static_cast<uint32_t>(PingMetric::Jitter));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::Jitter)) == L"Jitter");


    CHECK(ParseMetric(Category::Ping, L"ttl") == static_cast<uint32_t>(PingMetric::Ttl));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::Ttl)) == L"Ttl");


    CHECK(ParseMetric(Category::Ping, L"packetssent") == static_cast<uint32_t>(PingMetric::PacketsSent));

    CHECK(ParseMetric(Category::Ping, L"sent") == static_cast<uint32_t>(PingMetric::PacketsSent));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::PacketsSent)) == L"PacketsSent");


    CHECK(ParseMetric(Category::Ping, L"packetsreceived") == static_cast<uint32_t>(PingMetric::PacketsReceived));

    CHECK(ParseMetric(Category::Ping, L"received") == static_cast<uint32_t>(PingMetric::PacketsReceived));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::PacketsReceived)) == L"PacketsReceived");


    CHECK(ParseMetric(Category::Ping, L"resolvedip") == static_cast<uint32_t>(PingMetric::ResolvedIp));

    CHECK(ParseMetric(Category::Ping, L"ip") == static_cast<uint32_t>(PingMetric::ResolvedIp));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::ResolvedIp)) == L"ResolvedIp");


    CHECK(ParseMetric(Category::Ping, L"status") == static_cast<uint32_t>(PingMetric::Status));

    CHECK(MetricName(Category::Ping, static_cast<uint32_t>(PingMetric::Status)) == L"Status");


    CHECK(ParseMetric(Category::Ping, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Ping, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Ping, L"nonsense")    == MetricCount(Category::Ping));

    assert(ParseMetric(Category::Ping, L"  Usage ")    == ParseMetric(Category::Ping, L"usage"));  // trim + case




    // --- Storage

    CHECK(ParseMetric(Category::Storage, L"readbytes") == static_cast<uint32_t>(StorageMetric::ReadBytes));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::ReadBytes)) == L"ReadBytes");


    CHECK(ParseMetric(Category::Storage, L"writebytes") == static_cast<uint32_t>(StorageMetric::WriteBytes));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::WriteBytes)) == L"WriteBytes");


    CHECK(ParseMetric(Category::Storage, L"readspeed") == static_cast<uint32_t>(StorageMetric::ReadSpeed));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::ReadSpeed)) == L"ReadSpeed");


    CHECK(ParseMetric(Category::Storage, L"writespeed") == static_cast<uint32_t>(StorageMetric::WriteSpeed));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::WriteSpeed)) == L"WriteSpeed");


    CHECK(ParseMetric(Category::Storage, L"usedspace") == static_cast<uint32_t>(StorageMetric::UsedSpace));

    CHECK(ParseMetric(Category::Storage, L"used") == static_cast<uint32_t>(StorageMetric::UsedSpace));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::UsedSpace)) == L"UsedSpace");


    CHECK(ParseMetric(Category::Storage, L"freespace") == static_cast<uint32_t>(StorageMetric::FreeSpace));

    CHECK(ParseMetric(Category::Storage, L"free") == static_cast<uint32_t>(StorageMetric::FreeSpace));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::FreeSpace)) == L"FreeSpace");


    CHECK(ParseMetric(Category::Storage, L"totalspace") == static_cast<uint32_t>(StorageMetric::TotalSpace));

    CHECK(ParseMetric(Category::Storage, L"total") == static_cast<uint32_t>(StorageMetric::TotalSpace));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::TotalSpace)) == L"TotalSpace");


    CHECK(ParseMetric(Category::Storage, L"queuelength") == static_cast<uint32_t>(StorageMetric::QueueLength));

    CHECK(ParseMetric(Category::Storage, L"queue") == static_cast<uint32_t>(StorageMetric::QueueLength));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::QueueLength)) == L"QueueLength");


    CHECK(ParseMetric(Category::Storage, L"busypercent") == static_cast<uint32_t>(StorageMetric::BusyPercent));

    CHECK(ParseMetric(Category::Storage, L"busy") == static_cast<uint32_t>(StorageMetric::BusyPercent));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::BusyPercent)) == L"BusyPercent");


    CHECK(ParseMetric(Category::Storage, L"readspersec") == static_cast<uint32_t>(StorageMetric::ReadsPerSec));

    CHECK(ParseMetric(Category::Storage, L"readiops") == static_cast<uint32_t>(StorageMetric::ReadsPerSec));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::ReadsPerSec)) == L"ReadsPerSec");


    CHECK(ParseMetric(Category::Storage, L"writespersec") == static_cast<uint32_t>(StorageMetric::WritesPerSec));

    CHECK(ParseMetric(Category::Storage, L"writeiops") == static_cast<uint32_t>(StorageMetric::WritesPerSec));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::WritesPerSec)) == L"WritesPerSec");


    CHECK(ParseMetric(Category::Storage, L"temperature") == static_cast<uint32_t>(StorageMetric::Temperature));

    CHECK(ParseMetric(Category::Storage, L"temp") == static_cast<uint32_t>(StorageMetric::Temperature));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::Temperature)) == L"Temperature");


    CHECK(ParseMetric(Category::Storage, L"volumelabel") == static_cast<uint32_t>(StorageMetric::VolumeLabel));

    CHECK(ParseMetric(Category::Storage, L"label") == static_cast<uint32_t>(StorageMetric::VolumeLabel));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::VolumeLabel)) == L"VolumeLabel");


    CHECK(ParseMetric(Category::Storage, L"filesystem") == static_cast<uint32_t>(StorageMetric::FileSystem));

    CHECK(ParseMetric(Category::Storage, L"fs") == static_cast<uint32_t>(StorageMetric::FileSystem));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::FileSystem)) == L"FileSystem");


    CHECK(ParseMetric(Category::Storage, L"drivetype") == static_cast<uint32_t>(StorageMetric::DriveType));

    CHECK(ParseMetric(Category::Storage, L"type") == static_cast<uint32_t>(StorageMetric::DriveType));

    CHECK(MetricName(Category::Storage, static_cast<uint32_t>(StorageMetric::DriveType)) == L"DriveType");


    CHECK(ParseMetric(Category::Storage, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Storage, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Storage, L"nonsense")    == MetricCount(Category::Storage));

    assert(ParseMetric(Category::Storage, L"  Usage ")    == ParseMetric(Category::Storage, L"usage"));  // trim + case




    // --- Battery

    CHECK(ParseMetric(Category::Battery, L"chargelevel") == static_cast<uint32_t>(BatteryMetric::ChargeLevel));

    CHECK(ParseMetric(Category::Battery, L"charge") == static_cast<uint32_t>(BatteryMetric::ChargeLevel));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::ChargeLevel)) == L"ChargeLevel");


    CHECK(ParseMetric(Category::Battery, L"charging") == static_cast<uint32_t>(BatteryMetric::Charging));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Charging)) == L"Charging");


    CHECK(ParseMetric(Category::Battery, L"aconline") == static_cast<uint32_t>(BatteryMetric::AcOnline));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::AcOnline)) == L"AcOnline");


    CHECK(ParseMetric(Category::Battery, L"timetoempty") == static_cast<uint32_t>(BatteryMetric::TimeToEmpty));

    CHECK(ParseMetric(Category::Battery, L"timeleft") == static_cast<uint32_t>(BatteryMetric::TimeToEmpty));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::TimeToEmpty)) == L"TimeToEmpty");


    CHECK(ParseMetric(Category::Battery, L"rate") == static_cast<uint32_t>(BatteryMetric::Rate));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Rate)) == L"Rate");


    CHECK(ParseMetric(Category::Battery, L"voltage") == static_cast<uint32_t>(BatteryMetric::Voltage));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Voltage)) == L"Voltage");


    CHECK(ParseMetric(Category::Battery, L"remainingcapacity") == static_cast<uint32_t>(BatteryMetric::RemainingCapacity));

    CHECK(ParseMetric(Category::Battery, L"remaining") == static_cast<uint32_t>(BatteryMetric::RemainingCapacity));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::RemainingCapacity)) == L"RemainingCapacity");


    CHECK(ParseMetric(Category::Battery, L"fullchargecapacity") == static_cast<uint32_t>(BatteryMetric::FullChargeCapacity));

    CHECK(ParseMetric(Category::Battery, L"fullcapacity") == static_cast<uint32_t>(BatteryMetric::FullChargeCapacity));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::FullChargeCapacity)) == L"FullChargeCapacity");


    CHECK(ParseMetric(Category::Battery, L"designcapacity") == static_cast<uint32_t>(BatteryMetric::DesignCapacity));

    CHECK(ParseMetric(Category::Battery, L"design") == static_cast<uint32_t>(BatteryMetric::DesignCapacity));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::DesignCapacity)) == L"DesignCapacity");


    CHECK(ParseMetric(Category::Battery, L"wearlevel") == static_cast<uint32_t>(BatteryMetric::WearLevel));

    CHECK(ParseMetric(Category::Battery, L"wear") == static_cast<uint32_t>(BatteryMetric::WearLevel));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::WearLevel)) == L"WearLevel");


    CHECK(ParseMetric(Category::Battery, L"cyclecount") == static_cast<uint32_t>(BatteryMetric::CycleCount));

    CHECK(ParseMetric(Category::Battery, L"cycles") == static_cast<uint32_t>(BatteryMetric::CycleCount));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::CycleCount)) == L"CycleCount");


    CHECK(ParseMetric(Category::Battery, L"status") == static_cast<uint32_t>(BatteryMetric::Status));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Status)) == L"Status");


    CHECK(ParseMetric(Category::Battery, L"chemistry") == static_cast<uint32_t>(BatteryMetric::Chemistry));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Chemistry)) == L"Chemistry");


    CHECK(ParseMetric(Category::Battery, L"manufacturer") == static_cast<uint32_t>(BatteryMetric::Manufacturer));

    CHECK(ParseMetric(Category::Battery, L"mfr") == static_cast<uint32_t>(BatteryMetric::Manufacturer));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::Manufacturer)) == L"Manufacturer");


    CHECK(ParseMetric(Category::Battery, L"serialnumber") == static_cast<uint32_t>(BatteryMetric::SerialNumber));

    CHECK(ParseMetric(Category::Battery, L"serial") == static_cast<uint32_t>(BatteryMetric::SerialNumber));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::SerialNumber)) == L"SerialNumber");


    CHECK(ParseMetric(Category::Battery, L"devicename") == static_cast<uint32_t>(BatteryMetric::DeviceName));

    CHECK(ParseMetric(Category::Battery, L"name") == static_cast<uint32_t>(BatteryMetric::DeviceName));

    CHECK(MetricName(Category::Battery, static_cast<uint32_t>(BatteryMetric::DeviceName)) == L"DeviceName");


    CHECK(ParseMetric(Category::Battery, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Battery, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Battery, L"nonsense")    == MetricCount(Category::Battery));

    assert(ParseMetric(Category::Battery, L"  Usage ")    == ParseMetric(Category::Battery, L"usage"));  // trim + case




    // --- System

    CHECK(ParseMetric(Category::System, L"uptime") == static_cast<uint32_t>(SystemMetric::Uptime));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::Uptime)) == L"Uptime");


    CHECK(ParseMetric(Category::System, L"processcount") == static_cast<uint32_t>(SystemMetric::ProcessCount));

    CHECK(ParseMetric(Category::System, L"processes") == static_cast<uint32_t>(SystemMetric::ProcessCount));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::ProcessCount)) == L"ProcessCount");


    CHECK(ParseMetric(Category::System, L"threadcount") == static_cast<uint32_t>(SystemMetric::ThreadCount));

    CHECK(ParseMetric(Category::System, L"threads") == static_cast<uint32_t>(SystemMetric::ThreadCount));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::ThreadCount)) == L"ThreadCount");


    CHECK(ParseMetric(Category::System, L"handlecount") == static_cast<uint32_t>(SystemMetric::HandleCount));

    CHECK(ParseMetric(Category::System, L"handles") == static_cast<uint32_t>(SystemMetric::HandleCount));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::HandleCount)) == L"HandleCount");


    CHECK(ParseMetric(Category::System, L"osname") == static_cast<uint32_t>(SystemMetric::OsName));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::OsName)) == L"OsName");


    CHECK(ParseMetric(Category::System, L"osversion") == static_cast<uint32_t>(SystemMetric::OsVersion));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::OsVersion)) == L"OsVersion");


    CHECK(ParseMetric(Category::System, L"osbuild") == static_cast<uint32_t>(SystemMetric::OsBuild));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::OsBuild)) == L"OsBuild");


    CHECK(ParseMetric(Category::System, L"hostname") == static_cast<uint32_t>(SystemMetric::Hostname));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::Hostname)) == L"Hostname");


    CHECK(ParseMetric(Category::System, L"username") == static_cast<uint32_t>(SystemMetric::UserName));

    CHECK(ParseMetric(Category::System, L"user") == static_cast<uint32_t>(SystemMetric::UserName));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::UserName)) == L"UserName");


    CHECK(ParseMetric(Category::System, L"boottime") == static_cast<uint32_t>(SystemMetric::BootTime));

    CHECK(MetricName(Category::System, static_cast<uint32_t>(SystemMetric::BootTime)) == L"BootTime");


    CHECK(ParseMetric(Category::System, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::System, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::System, L"nonsense")    == MetricCount(Category::System));

    assert(ParseMetric(Category::System, L"  Usage ")    == ParseMetric(Category::System, L"usage"));  // trim + case




    // --- Motherboard

    CHECK(ParseMetric(Category::Motherboard, L"manufacturer") == static_cast<uint32_t>(MotherboardMetric::Manufacturer));

    CHECK(ParseMetric(Category::Motherboard, L"mfr") == static_cast<uint32_t>(MotherboardMetric::Manufacturer));

    CHECK(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::Manufacturer)) == L"Manufacturer");


    CHECK(ParseMetric(Category::Motherboard, L"product") == static_cast<uint32_t>(MotherboardMetric::Product));

    CHECK(ParseMetric(Category::Motherboard, L"model") == static_cast<uint32_t>(MotherboardMetric::Product));

    CHECK(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::Product)) == L"Product");


    CHECK(ParseMetric(Category::Motherboard, L"serialnumber") == static_cast<uint32_t>(MotherboardMetric::SerialNumber));

    CHECK(ParseMetric(Category::Motherboard, L"serial") == static_cast<uint32_t>(MotherboardMetric::SerialNumber));

    CHECK(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::SerialNumber)) == L"SerialNumber");


    CHECK(ParseMetric(Category::Motherboard, L"biosversion") == static_cast<uint32_t>(MotherboardMetric::BiosVersion));

    CHECK(ParseMetric(Category::Motherboard, L"bios") == static_cast<uint32_t>(MotherboardMetric::BiosVersion));

    CHECK(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::BiosVersion)) == L"BiosVersion");


    CHECK(ParseMetric(Category::Motherboard, L"biosdate") == static_cast<uint32_t>(MotherboardMetric::BiosDate));

    CHECK(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::BiosDate)) == L"BiosDate");


    CHECK(ParseMetric(Category::Motherboard, L"systemmanufacturer") == static_cast<uint32_t>(MotherboardMetric::SystemManufacturer));

    CHECK(ParseMetric(Category::Motherboard, L"sysmfr") == static_cast<uint32_t>(MotherboardMetric::SystemManufacturer));

    CHECK(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::SystemManufacturer)) == L"SystemManufacturer");


    CHECK(ParseMetric(Category::Motherboard, L"systemproduct") == static_cast<uint32_t>(MotherboardMetric::SystemProduct));

    CHECK(ParseMetric(Category::Motherboard, L"sysmodel") == static_cast<uint32_t>(MotherboardMetric::SystemProduct));

    CHECK(MetricName(Category::Motherboard, static_cast<uint32_t>(MotherboardMetric::SystemProduct)) == L"SystemProduct");


    CHECK(ParseMetric(Category::Motherboard, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Motherboard, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Motherboard, L"nonsense")    == MetricCount(Category::Motherboard));

    assert(ParseMetric(Category::Motherboard, L"  Usage ")    == ParseMetric(Category::Motherboard, L"usage"));  // trim + case




    // --- Display

    CHECK(ParseMetric(Category::Display, L"width") == static_cast<uint32_t>(DisplayMetric::Width));

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Width)) == L"Width");


    CHECK(ParseMetric(Category::Display, L"height") == static_cast<uint32_t>(DisplayMetric::Height));

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Height)) == L"Height");


    CHECK(ParseMetric(Category::Display, L"refreshrate") == static_cast<uint32_t>(DisplayMetric::RefreshRate));

    CHECK(ParseMetric(Category::Display, L"refresh") == static_cast<uint32_t>(DisplayMetric::RefreshRate));

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::RefreshRate)) == L"RefreshRate");


    CHECK(ParseMetric(Category::Display, L"bitsperpixel") == static_cast<uint32_t>(DisplayMetric::BitsPerPixel));

    CHECK(ParseMetric(Category::Display, L"bpp") == static_cast<uint32_t>(DisplayMetric::BitsPerPixel));

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::BitsPerPixel)) == L"BitsPerPixel");


    CHECK(ParseMetric(Category::Display, L"primary") == static_cast<uint32_t>(DisplayMetric::Primary));

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Primary)) == L"Primary");


    // "count" is answered centrally before the table is consulted, in every category -
    // so DisplayMetric::Count is unreachable by name, exactly as it was unreachable past
    // the old if-chain. The name still has to print, which the next line checks.
    CHECK(ParseMetric(Category::Display, L"count") == METRIC_DEVICE_COUNT);

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Count)) == L"Count");


    CHECK(ParseMetric(Category::Display, L"name") == static_cast<uint32_t>(DisplayMetric::Name));

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Name)) == L"Name");


    CHECK(ParseMetric(Category::Display, L"devicename") == static_cast<uint32_t>(DisplayMetric::DeviceName));

    CHECK(ParseMetric(Category::Display, L"device") == static_cast<uint32_t>(DisplayMetric::DeviceName));

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::DeviceName)) == L"DeviceName");


    CHECK(ParseMetric(Category::Display, L"resolution") == static_cast<uint32_t>(DisplayMetric::Resolution));

    CHECK(ParseMetric(Category::Display, L"res") == static_cast<uint32_t>(DisplayMetric::Resolution));

    CHECK(MetricName(Category::Display, static_cast<uint32_t>(DisplayMetric::Resolution)) == L"Resolution");


    CHECK(ParseMetric(Category::Display, L"count")       == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Display, L"devicecount") == METRIC_DEVICE_COUNT);

    CHECK(ParseMetric(Category::Display, L"nonsense")    == MetricCount(Category::Display));

    assert(ParseMetric(Category::Display, L"  Usage ")    == ParseMetric(Category::Display, L"usage"));  // trim + case




    // ProviderStatus is reserved in every category, like Count - and must not have taken
    // "status" with it, which Battery and Ping already use for a string metric of their own
    // (asserted per category above).
    for (Category c : { Category::GPU, Category::CPU, Category::Memory, Category::Network,
                        Category::Storage, Category::Ping, Category::Battery, Category::System,
                        Category::Motherboard, Category::Display })
    {
        CHECK(ParseMetric(c, L"ProviderStatus") == METRIC_PROVIDER_STATUS);
        CHECK(ParseMetric(c, L"providerstatus") == METRIC_PROVIDER_STATUS);
        CHECK(ParseMetric(c, L"provider")       == METRIC_PROVIDER_STATUS);
        CHECK(MetricName(c, METRIC_PROVIDER_STATUS) == L"Metric[4294967293]");  // not a table row
    }

    CHECK(ParseMetric(Category::Unknown, L"usage") == static_cast<uint32_t>(-1));



    printf("metric_parse_check: %d assertions passed\n", checked);
    return 0;

}

