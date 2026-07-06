#include "Providers/WinApi/WinApiBatteryProvider.h"
#include "Utils/WmiUtil.h"
#include "Types/BatteryMetric.h"
#include "Utils/Debug.h"

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

    m_wmiServices = Wmi::Connect(L"ROOT\\WMI", m_comInitialized); // best-effort
    ReadStaticData();

    LOG_STARTUP(L"WinApiBatteryProvider: initialized");
    return true;
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
    IWbemClassObject* obj = Wmi::QueryFirst(m_wmiServices, L"SELECT * FROM BatteryStaticData");
    if (!obj)
        return;

    double design = 0.0;
    if (Wmi::GetU32(obj, L"DesignedCapacity", design)) // mWh
        m_designCapacity = design;

    m_chemistry    = GetChemistry(obj);
    m_manufacturer = Wmi::GetStr(obj, L"ManufactureName");
    m_serialNumber = Wmi::GetStr(obj, L"SerialNumber");
    m_deviceName   = Wmi::GetStr(obj, L"DeviceName");

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
    double value        = 0.0;
    double fullCapacity = 0.0;
    bool   haveFull     = false;

    if (IWbemClassObject* obj = Wmi::QueryFirst(m_wmiServices, L"SELECT * FROM BatteryStatus"))
    {
        if (Wmi::GetU32(obj, L"Voltage", value))           // mV
            snap.Set(static_cast<uint32_t>(BatteryMetric::Voltage), value);
        if (Wmi::GetU32(obj, L"RemainingCapacity", value)) // mWh
            snap.Set(static_cast<uint32_t>(BatteryMetric::RemainingCapacity), value);

        // Rate is already signed (+ charging / - discharging), mW.
        VARIANT vr; VariantInit(&vr);
        if (SUCCEEDED(obj->Get(L"Rate", 0, &vr, nullptr, nullptr)) && vr.vt == VT_I4)
            snap.Set(static_cast<uint32_t>(BatteryMetric::Rate), static_cast<double>(vr.lVal));
        VariantClear(&vr);

        obj->Release();
    }

    if (IWbemClassObject* obj = Wmi::QueryFirst(m_wmiServices, L"SELECT * FROM BatteryFullChargedCapacity"))
    {
        haveFull = Wmi::GetU32(obj, L"FullChargedCapacity", fullCapacity); // mWh
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

    if (IWbemClassObject* obj = Wmi::QueryFirst(m_wmiServices, L"SELECT * FROM BatteryCycleCount"))
    {
        if (Wmi::GetU32(obj, L"CycleCount", value))
            snap.Set(static_cast<uint32_t>(BatteryMetric::CycleCount), value);
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
