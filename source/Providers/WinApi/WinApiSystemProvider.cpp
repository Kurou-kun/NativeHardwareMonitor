#include "Providers/WinApi/WinApiSystemProvider.h"
#include "Types/SystemMetric.h"
#include "Utils/Debug.h"

#include <psapi.h>
#include <comdef.h>
#include <lmcons.h> // UNLEN

#pragma comment(lib, "wbemuuid.lib")

WinApiSystemProvider::~WinApiSystemProvider()
{
    if (m_wmiServices) m_wmiServices->Release();
    if (m_comInitialized) CoUninitialize();
}

bool WinApiSystemProvider::Initialize()
{
    InitWmi();      // best-effort — OS identity strings just stay unsupported if unavailable
    ReadOsInfo();
    ReadIdentity(); // Win32 identity, no WMI dependency

    LOG_STARTUP(L"WinApiSystemProvider: initialized");
    return true;
}

void WinApiSystemProvider::InitWmi()
{
    // Same COM-apartment handling as the other WMI providers: RPC_E_CHANGED_MODE is fine.
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

void WinApiSystemProvider::ReadOsInfo()
{
    if (!m_wmiServices)
        return;

    IEnumWbemClassObject* enumerator = nullptr;
    HRESULT hr = m_wmiServices->ExecQuery(_bstr_t(L"WQL"),
        _bstr_t(L"SELECT Caption, Version, BuildNumber FROM Win32_OperatingSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);

    if (FAILED(hr) || !enumerator)
        return;

    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;
    if (enumerator->Next(WBEM_INFINITE, 1, &obj, &returned) == S_OK && obj)
    {
        auto getStr = [&](const wchar_t* name) -> std::wstring {
            VARIANT v; VariantInit(&v);
            std::wstring out;
            if (SUCCEEDED(obj->Get(name, 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR)
                out = TrimBstr(v.bstrVal);
            VariantClear(&v);
            return out;
        };

        m_osName    = getStr(L"Caption");
        m_osVersion = getStr(L"Version");
        m_osBuild   = getStr(L"BuildNumber");

        obj->Release();
    }
    enumerator->Release();
}

void WinApiSystemProvider::ReadIdentity()
{
    wchar_t nameBuf[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD nameLen = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(nameBuf, &nameLen))
        m_hostname = nameBuf;

    wchar_t userBuf[UNLEN + 1] = {};
    DWORD userLen = UNLEN + 1;
    if (GetUserNameW(userBuf, &userLen))
        m_userName = userBuf; // includes trailing NUL length; assigning C-string is fine

    // Boot time = wall clock now - uptime. Fixed for the session, so compute once.
    FILETIME nowFt;
    GetSystemTimeAsFileTime(&nowFt);
    ULARGE_INTEGER now;
    now.LowPart  = nowFt.dwLowDateTime;
    now.HighPart = nowFt.dwHighDateTime;
    now.QuadPart -= static_cast<uint64_t>(GetTickCount64()) * 10000ull; // ms -> 100ns ticks

    FILETIME bootUtc;
    bootUtc.dwLowDateTime  = now.LowPart;
    bootUtc.dwHighDateTime = now.HighPart;

    FILETIME bootLocal;
    SYSTEMTIME st{};
    if (FileTimeToLocalFileTime(&bootUtc, &bootLocal) && FileTimeToSystemTime(&bootLocal, &st))
    {
        wchar_t buf[32];
        swprintf(buf, 32, L"%04u-%02u-%02u %02u:%02u:%02u",
                 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        m_bootTime = buf;
    }
}

uint32_t WinApiSystemProvider::GetDeviceCount() const
{
    return 1;
}

void WinApiSystemProvider::GatherSnapshot(uint32_t deviceIndex, Snapshot& snap)
{
    if (deviceIndex != 0)
        return;

    snap.Set(static_cast<uint32_t>(SystemMetric::Uptime),
             static_cast<double>(GetTickCount64()) / 1000.0);

    PERFORMANCE_INFORMATION perf{ sizeof(perf) };
    if (GetPerformanceInfo(&perf, sizeof(perf)))
    {
        snap.Set(static_cast<uint32_t>(SystemMetric::ProcessCount), static_cast<double>(perf.ProcessCount));
        snap.Set(static_cast<uint32_t>(SystemMetric::ThreadCount),  static_cast<double>(perf.ThreadCount));
        snap.Set(static_cast<uint32_t>(SystemMetric::HandleCount),  static_cast<double>(perf.HandleCount));
    }
}

bool WinApiSystemProvider::GetString(uint32_t metricId, uint32_t deviceIndex, std::wstring& out)
{
    if (deviceIndex != 0)
        return false;

    switch (static_cast<SystemMetric>(metricId))
    {
    case SystemMetric::OsName:    if (m_osName.empty())    return false; out = m_osName;    return true;
    case SystemMetric::OsVersion: if (m_osVersion.empty()) return false; out = m_osVersion; return true;
    case SystemMetric::OsBuild:   if (m_osBuild.empty())   return false; out = m_osBuild;   return true;
    case SystemMetric::Hostname:  if (m_hostname.empty())  return false; out = m_hostname;  return true;
    case SystemMetric::UserName:  if (m_userName.empty())  return false; out = m_userName;  return true;
    case SystemMetric::BootTime:  if (m_bootTime.empty())  return false; out = m_bootTime;  return true;
    default:                      return false;
    }
}
