#pragma once

#include <cstdint>

// All string metrics — static board/BIOS identity. Nothing numeric is
// meaningful here, so GetValue always reports unsupported (-1).
enum class MotherboardMetric : uint32_t
{
    Manufacturer,       // board maker, Win32_BaseBoard.Manufacturer
    Product,            // board model, Win32_BaseBoard.Product
    SerialNumber,       // Win32_BaseBoard.SerialNumber (often empty w/o admin)
    BiosVersion,        // Win32_BIOS.SMBIOSBIOSVersion
    BiosDate,           // Win32_BIOS.ReleaseDate -> "YYYY-MM-DD"
    SystemManufacturer, // Win32_ComputerSystem.Manufacturer, e.g. "ASUS"
    SystemProduct,      // Win32_ComputerSystem.Model
    Unknown
};
