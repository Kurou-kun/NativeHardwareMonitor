#include "Providers/WinApi/WinApiSystemProvider.h"
#include "Utils/WmiUtil.h"
#include "Types/SystemMetric.h"
#include "Utils/Debug.h"

#include <psapi.h>
#include <lmcons.h> // UNLEN

WinApiSystemProvider::~WinApiSystemProvider()
{
    if (m_wmiServices) m_wmiServices->Release();
    if (m_comInitialized) CoUninitialize();
}

bool WinApiSystemProvider::Initialize()
{
    m_wmiServices = Wmi::Connect(L"ROOT\\CIMV2", m_comInitialized);
    ReadOsInfo();
    ReadIdentity(); // Win32 identity, no WMI dependency

    LOG_STARTUP(L"WinApiSystemProvider: initialized");
    return true;
}

void WinApiSystemProvider::ReadOsInfo()
{
    IWbemClassObject* obj = Wmi::QueryFirst(m_wmiServices,
        L"SELECT Caption, Version, BuildNumber FROM Win32_OperatingSystem");
    if (!obj)
        return;

    m_osName    = Wmi::GetStr(obj, L"Caption");
    m_osVersion = Wmi::GetStr(obj, L"Version");
    m_osBuild   = Wmi::GetStr(obj, L"BuildNumber");

    obj->Release();
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
        m_userName = userBuf;

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
