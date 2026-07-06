#pragma once

#include "Core/IProvider.h"
#include "Types/Snapshot.h"

#include <windows.h>
#include <wbemidl.h>
#include <string>

// Single "motherboard" device (index 0). Static board/BIOS identity, read once
// at Initialize from WMI (Win32_BaseBoard / Win32_BIOS / Win32_ComputerSystem).
// String-only: GatherSnapshot sets nothing, so numeric reads report unsupported.
class WinApiMotherboardProvider : public IProvider
{
public:
    ~WinApiMotherboardProvider();

    bool     Initialize() override;
    uint32_t GetDeviceCount() const override;
    void     GatherSnapshot(uint32_t deviceIndex, Snapshot& snap) override;
    bool     GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out) override;

private:
    IWbemServices* m_wmiServices    = nullptr; // ROOT\CIMV2
    bool           m_comInitialized = false;

    std::wstring m_manufacturer;
    std::wstring m_product;
    std::wstring m_serialNumber;
    std::wstring m_biosVersion;
    std::wstring m_biosDate;
    std::wstring m_systemManufacturer;
    std::wstring m_systemProduct;

    void ReadIdentity();
};
