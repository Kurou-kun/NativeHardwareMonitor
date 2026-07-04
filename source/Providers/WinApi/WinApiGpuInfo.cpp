#include "Providers/WinApi/WinApiGpuInfo.h"

#include <windows.h>
#include <dxgi.h>
#include <cstdio>
#include <cwchar>

bool WinApiGpuInfo::EnsureEnumerated()
{
    if (m_enumerated)
        return true;
    m_enumerated = true; // only try once, even on failure

    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)) || !factory)
        return false;

    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) == S_OK; ++i)
    {
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
            m_adapters.push_back({ desc.Description, desc.VendorId });

        adapter->Release();
        adapter = nullptr;
    }

    factory->Release();
    return !m_adapters.empty();
}

bool WinApiGpuInfo::GetName(uint32_t pciVendorId, uint32_t ordinal, std::wstring& out)
{
    if (!EnsureEnumerated())
        return false;

    uint32_t seen = 0;
    for (auto& a : m_adapters)
    {
        if (a.vendorId == pciVendorId && seen++ == ordinal)
        {
            out = a.name;
            return true;
        }
    }
    return false;
}

// Matches by "Nth Display class registry subkey whose MatchingDeviceID names this
// PCI vendor" — the same "same enumeration order across sources" assumption
// GpuResolver already accepts for its primary/backup provider pairing.
bool WinApiGpuInfo::GetDriverVersion(uint32_t pciVendorId, uint32_t ordinal, std::wstring& out)
{
    wchar_t venTag[16];
    swprintf_s(venTag, L"VEN_%04X", pciVendorId);

    HKEY classKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
        0, KEY_READ, &classKey) != ERROR_SUCCESS)
        return false;

    bool     found = false;
    uint32_t seen  = 0;

    for (DWORD i = 0; !found; ++i)
    {
        wchar_t subName[16];
        DWORD   subNameLen = _countof(subName);
        if (RegEnumKeyExW(classKey, i, subName, &subNameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
            break;

        HKEY subKey;
        if (RegOpenKeyExW(classKey, subName, 0, KEY_READ, &subKey) != ERROR_SUCCESS)
            continue;

        wchar_t matchId[256] = {};
        DWORD   matchSize    = sizeof(matchId);
        if (RegQueryValueExW(subKey, L"MatchingDeviceID", nullptr, nullptr, (LPBYTE)matchId, &matchSize) == ERROR_SUCCESS)
        {
            _wcsupr_s(matchId, _countof(matchId));

            if (wcsstr(matchId, venTag) != nullptr && seen++ == ordinal)
            {
                wchar_t verBuf[64] = {};
                DWORD   verSize    = sizeof(verBuf);
                if (RegQueryValueExW(subKey, L"DriverVersion", nullptr, nullptr, (LPBYTE)verBuf, &verSize) == ERROR_SUCCESS)
                {
                    out   = verBuf;
                    found = true;
                }
            }
        }

        RegCloseKey(subKey);
    }

    RegCloseKey(classKey);
    return found;
}
