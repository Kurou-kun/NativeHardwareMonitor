#include "Providers/WinApi/WinApiBatteryProvider.h"
#include "Types/BatteryMetric.h"
#include "Utils/Debug.h"

#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")

WinApiBatteryProvider::~WinApiBatteryProvider()
{
    if (m_wmiServices) m_wmiServices->Release();
    if (m_comInitialized) CoUninitialize();
}

bool WinApiBatteryProvider::Initialize()
{
    SYSTEM_POWER_STATUS sps{};
    if (!GetSystemPowerStatus(&sps))
    {
        LOG_STARTUP(L"WinApiBatteryProvider: GetSystemPowerStatus failed");
        return false;
    }

    InitWmi();        // best-effort — capacity/identity just stay unsupported if unavailable
    ReadStaticData(); // best-effort battery identity

    LOG_STARTUP(L"WinApiBatteryProvider: initialized");
    return true;
}

void WinApiBatteryProvider::InitWmi()
{
    // Same COM-apartment handling as WinApiMemoryProvider: RPC_E_CHANGED_MODE is fine.
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    m_comInitialized = SUCCEEDED(coInit);

    IWbemLocator* locator = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                 IID_IWbemLocator, (LPVOID*)&locator)) || !locator)
        return;

    HRESULT hr = locator->ConnectServer(_bstr_t(L"ROOT\\WMI"), nullptr, nullptr, nullptr,
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

IWbemClassObject* WinApiBatteryProvider::QueryFirst(const wchar_t* wql)
{
    if (!m_wmiServices)
        return nullptr;

    IEnumWbemClassObject* enumerator = nullptr;
    HRESULT hr = m_wmiServices->ExecQuery(_bstr_t(L"WQL"), _bstr_t(wql),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);

    if (FAILED(hr) || !enumerator)
        return nullptr;

    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;
    enumerator->Next(WBEM_INFINITE, 1, &obj, &returned); // first instance only
    enumerator->Release();

    return returned == 1 ? obj : nullptr;
}

static double GetU32(IWbemClassObject* obj, const wchar_t* name, bool& ok)
{
    VARIANT v; VariantInit(&v);
    ok = false;
    double out = 0.0;
    if (SUCCEEDED(obj->Get(name, 0, &v, nullptr, nullptr)))
    {
        if      (v.vt == VT_I4)  { out = v.lVal;  ok = true; }
        else if (v.vt == VT_UI4) { out = v.ulVal; ok = true; }
    }
    VariantClear(&v);
    return out;
}

static std::wstring GetStr(IWbemClassObject* obj, const wchar_t* name)
{
    VARIANT v; VariantInit(&v);
    std::wstring out;
    if (SUCCEEDED(obj->Get(name, 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR && v.bstrVal)
        out = v.bstrVal;
    VariantClear(&v);
    return out;
}

// BatteryStaticData.Chemistry is a uint8[] of ASCII bytes, e.g. {'L','I','O','N'}.
static std::wstring GetChemistry(IWbemClassObject* obj)
{
    VARIANT v; VariantInit(&v);
    std::wstring out;
    if (SUCCEEDED(obj->Get(L"Chemistry", 0, &v, nullptr, nullptr)) && (v.vt == (VT_ARRAY | VT_UI1)) && v.parray)
    {
        SAFEARRAY* arr = v.parray;
        LONG lbound = 0, ubound = -1;
        SafeArrayGetLBound(arr, 1, &lbound);
        SafeArrayGetUBound(arr, 1, &ubound);
        for (LONG i = lbound; i <= ubound; ++i)
        {
            BYTE b = 0;
            if (SUCCEEDED(SafeArrayGetElement(arr, &i, &b)) && b != 0)
                out += static_cast<wchar_t>(b);
        }
    }
    VariantClear(&v);
    return out;
}

void WinApiBatteryProvider::ReadStaticData()
{
    IWbemClassObject* obj = QueryFirst(L"SELECT * FROM BatteryStaticData");
    if (!obj)
        return;

    bool ok = false;
    double design = GetU32(obj, L"DesignedCapacity", ok); // mWh
    if (ok) m_designCapacity = design;

    m_chemistry    = GetChemistry(obj);
    m_manufacturer = GetStr(obj, L"ManufactureName");
    m_serialNumber = GetStr(obj, L"SerialNumber");
    m_deviceName   = GetStr(obj, L"DeviceName");

    obj->Release();
}

uint32_t WinApiBatteryProvider::GetDeviceCount() const
{
    return 1; // always one logical battery slot, even with no physical battery
}

void WinApiBatteryProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex != 0)
        return;

    SYSTEM_POWER_STATUS sps{};
    if (GetSystemPowerStatus(&sps))
    {
        const bool noBattery = (sps.BatteryFlag & 128) != 0;
        const bool charging  = (sps.BatteryFlag & 8) != 0;
        const bool acOnline  = sps.ACLineStatus == 1;

        if (sps.ACLineStatus != 255)
            snap.Set(static_cast<uint32_t>(BatteryMetric::AcOnline), acOnline ? 1.0 : 0.0);

        if (!noBattery)
        {
            if (sps.BatteryLifePercent != 255)
                snap.Set(static_cast<uint32_t>(BatteryMetric::ChargeLevel), sps.BatteryLifePercent);

            snap.Set(static_cast<uint32_t>(BatteryMetric::Charging), charging ? 1.0 : 0.0);

            // BatteryLifeTime: seconds to empty; 0xFFFFFFFF = unknown or plugged in.
            if (sps.BatteryLifeTime != static_cast<DWORD>(-1))
                snap.Set(static_cast<uint32_t>(BatteryMetric::TimeToEmpty),
                         static_cast<double>(sps.BatteryLifeTime));
        }

        // ponytail: coarse status heuristic from the power flags, good enough for a label.
        if (noBattery)
            m_statusText = L"AC";
        else if (charging)
            m_statusText = L"Charging";
        else if (acOnline)
            m_statusText = (sps.BatteryLifePercent != 255 && sps.BatteryLifePercent >= 100) ? L"Full" : L"AC";
        else
            m_statusText = L"Discharging";
    }

    GatherWmi(snap);
}

void WinApiBatteryProvider::GatherWmi(Snapshot& snap)
{
    double fullCapacity = 0.0;
    bool   haveFull     = false;

    if (IWbemClassObject* obj = QueryFirst(L"SELECT * FROM BatteryStatus"))
    {
        bool ok = false;

        double voltage = GetU32(obj, L"Voltage", ok); // mV
        if (ok) snap.Set(static_cast<uint32_t>(BatteryMetric::Voltage), voltage);

        double remaining = GetU32(obj, L"RemainingCapacity", ok); // mWh
        if (ok) snap.Set(static_cast<uint32_t>(BatteryMetric::RemainingCapacity), remaining);

        // Rate is already signed (+ charging / - discharging), mW.
        VARIANT vr; VariantInit(&vr);
        if (SUCCEEDED(obj->Get(L"Rate", 0, &vr, nullptr, nullptr)) && vr.vt == VT_I4)
            snap.Set(static_cast<uint32_t>(BatteryMetric::Rate), static_cast<double>(vr.lVal));
        VariantClear(&vr);

        obj->Release();
    }

    if (IWbemClassObject* obj = QueryFirst(L"SELECT * FROM BatteryFullChargedCapacity"))
    {
        fullCapacity = GetU32(obj, L"FullChargedCapacity", haveFull); // mWh
        if (haveFull)
            snap.Set(static_cast<uint32_t>(BatteryMetric::FullChargeCapacity), fullCapacity);
        obj->Release();
    }

    if (m_designCapacity > 0.0)
    {
        snap.Set(static_cast<uint32_t>(BatteryMetric::DesignCapacity), m_designCapacity);
        if (haveFull && fullCapacity > 0.0)
            snap.Set(static_cast<uint32_t>(BatteryMetric::WearLevel),
                     100.0 * (1.0 - fullCapacity / m_designCapacity));
    }

    if (IWbemClassObject* obj = QueryFirst(L"SELECT * FROM BatteryCycleCount"))
    {
        bool ok = false;
        double cycles = GetU32(obj, L"CycleCount", ok);
        if (ok) snap.Set(static_cast<uint32_t>(BatteryMetric::CycleCount), cycles);
        obj->Release();
    }
}

bool WinApiBatteryProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex != 0)
        return false;

    switch (static_cast<BatteryMetric>(metricId))
    {
    case BatteryMetric::Status:       if (m_statusText.empty())   return false; out = m_statusText;   return true;
    case BatteryMetric::Chemistry:    if (m_chemistry.empty())    return false; out = m_chemistry;    return true;
    case BatteryMetric::Manufacturer: if (m_manufacturer.empty()) return false; out = m_manufacturer; return true;
    case BatteryMetric::SerialNumber: if (m_serialNumber.empty()) return false; out = m_serialNumber; return true;
    case BatteryMetric::DeviceName:   if (m_deviceName.empty())   return false; out = m_deviceName;   return true;
    default:                          return false;
    }
}
