#include "Providers/WinApi/WinApiMotherboardProvider.h"
#include "Types/MotherboardMetric.h"
#include "Utils/Debug.h"

#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")

WinApiMotherboardProvider::~WinApiMotherboardProvider()
{
    if (m_wmiServices) m_wmiServices->Release();
    if (m_comInitialized) CoUninitialize();
}

bool WinApiMotherboardProvider::Initialize()
{
    InitWmi();
    ReadIdentity();

    LOG_STARTUP(L"WinApiMotherboardProvider: initialized");
    return true;
}

void WinApiMotherboardProvider::InitWmi()
{
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    m_comInitialized = SUCCEEDED(coInit);

    IWbemLocator* locator = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                 IID_IWbemLocator, (LPVOID*)&locator)) || !locator)
        return;

    HRESULT hr = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr,
                                         0, nullptr, nullptr, &m_wmiServices);
    if (FAILED(hr) || !m_wmiServices)
    {
        m_wmiServices = nullptr;
        locator->Release();
        return;
    }

    CoSetProxyBlanket(m_wmiServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                       RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    locator->Release();
}

static std::wstring TrimBstr(const wchar_t* s)
{
    if (!s) return L"";
    std::wstring v = s;
    size_t start = v.find_first_not_of(L' ');
    size_t end   = v.find_last_not_of(L' ');
    return start == std::wstring::npos ? L"" : v.substr(start, end - start + 1);
}

// Runs a single-row WQL query and returns the named string field (trimmed).
static std::wstring QueryStr(IWbemServices* svc, const wchar_t* wql, const wchar_t* field)
{
    if (!svc) return L"";

    IEnumWbemClassObject* enumerator = nullptr;
    HRESULT hr = svc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(wql),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);

    if (FAILED(hr) || !enumerator)
        return L"";

    std::wstring out;
    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;
    if (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK && obj)
    {
        VARIANT v; VariantInit(&v);
        if (SUCCEEDED(obj->Get(field, 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR)
            out = TrimBstr(v.bstrVal);
        VariantClear(&v);
        obj->Release();
    }
    enumerator->Release();
    return out;
}

void WinApiMotherboardProvider::ReadIdentity()
{
    if (!m_wmiServices)
        return;

    m_manufacturer       = QueryStr(m_wmiServices, L"SELECT Manufacturer FROM Win32_BaseBoard", L"Manufacturer");
    m_product            = QueryStr(m_wmiServices, L"SELECT Product FROM Win32_BaseBoard", L"Product");
    m_serialNumber       = QueryStr(m_wmiServices, L"SELECT SerialNumber FROM Win32_BaseBoard", L"SerialNumber");
    m_biosVersion        = QueryStr(m_wmiServices, L"SELECT SMBIOSBIOSVersion FROM Win32_BIOS", L"SMBIOSBIOSVersion");
    m_systemManufacturer = QueryStr(m_wmiServices, L"SELECT Manufacturer FROM Win32_ComputerSystem", L"Manufacturer");
    m_systemProduct      = QueryStr(m_wmiServices, L"SELECT Model FROM Win32_ComputerSystem", L"Model");

    // ReleaseDate is a WMI datetime (yyyymmddHHMMSS...) — keep just the date as YYYY-MM-DD.
    std::wstring rawDate = QueryStr(m_wmiServices, L"SELECT ReleaseDate FROM Win32_BIOS", L"ReleaseDate");
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
