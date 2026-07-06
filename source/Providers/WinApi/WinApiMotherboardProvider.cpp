#include "Providers/WinApi/WinApiMotherboardProvider.h"
#include "Utils/WmiUtil.h"
#include "Types/MotherboardMetric.h"
#include "Utils/Debug.h"

WinApiMotherboardProvider::~WinApiMotherboardProvider()
{
    if (m_wmiServices) m_wmiServices->Release();
    if (m_comInitialized) CoUninitialize();
}

bool WinApiMotherboardProvider::Initialize()
{
    m_wmiServices = Wmi::Connect(L"ROOT\\CIMV2", m_comInitialized);
    ReadIdentity();

    LOG_STARTUP(L"WinApiMotherboardProvider: initialized");
    return true;
}

void WinApiMotherboardProvider::ReadIdentity()
{
    if (!m_wmiServices)
        return;

    m_manufacturer       = Wmi::QueryStr(m_wmiServices, L"SELECT Manufacturer FROM Win32_BaseBoard", L"Manufacturer");
    m_product            = Wmi::QueryStr(m_wmiServices, L"SELECT Product FROM Win32_BaseBoard", L"Product");
    m_serialNumber       = Wmi::QueryStr(m_wmiServices, L"SELECT SerialNumber FROM Win32_BaseBoard", L"SerialNumber");
    m_biosVersion        = Wmi::QueryStr(m_wmiServices, L"SELECT SMBIOSBIOSVersion FROM Win32_BIOS", L"SMBIOSBIOSVersion");
    m_systemManufacturer = Wmi::QueryStr(m_wmiServices, L"SELECT Manufacturer FROM Win32_ComputerSystem", L"Manufacturer");
    m_systemProduct      = Wmi::QueryStr(m_wmiServices, L"SELECT Model FROM Win32_ComputerSystem", L"Model");

    // ReleaseDate is a WMI datetime (yyyymmddHHMMSS...) — keep just the date as YYYY-MM-DD.
    std::wstring rawDate = Wmi::QueryStr(m_wmiServices, L"SELECT ReleaseDate FROM Win32_BIOS", L"ReleaseDate");
    if (rawDate.size() >= 8)
        m_biosDate = rawDate.substr(0, 4) + L"-" + rawDate.substr(4, 2) + L"-" + rawDate.substr(6, 2);
}

uint32_t WinApiMotherboardProvider::GetDeviceCount() const
{
    return 1;
}

void WinApiMotherboardProvider::GatherSnapshot(uint32_t /*deviceIndex*/, Snapshot& /*snap*/)
{
    // ponytail: nothing numeric to gather — all metrics are static strings.
}

bool WinApiMotherboardProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex != 0)
        return false;

    switch (static_cast<MotherboardMetric>(metricId))
    {
    case MotherboardMetric::Manufacturer:       if (m_manufacturer.empty())       return false; out = m_manufacturer;       return true;
    case MotherboardMetric::Product:            if (m_product.empty())            return false; out = m_product;            return true;
    case MotherboardMetric::SerialNumber:       if (m_serialNumber.empty())       return false; out = m_serialNumber;       return true;
    case MotherboardMetric::BiosVersion:        if (m_biosVersion.empty())        return false; out = m_biosVersion;        return true;
    case MotherboardMetric::BiosDate:           if (m_biosDate.empty())           return false; out = m_biosDate;           return true;
    case MotherboardMetric::SystemManufacturer: if (m_systemManufacturer.empty()) return false; out = m_systemManufacturer; return true;
    case MotherboardMetric::SystemProduct:      if (m_systemProduct.empty())      return false; out = m_systemProduct;      return true;
    default:                                    return false;
    }
}
