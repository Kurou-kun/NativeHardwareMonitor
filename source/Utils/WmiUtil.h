#pragma once

#include <windows.h>
#include <wbemidl.h>
#include <string>

// Shared COM/WMI plumbing — every WinApi provider was inlining the same
// connect + VARIANT-extraction boilerplate. All best-effort: on any failure
// the getters return empty/false so callers just report the metric unsupported.
namespace Wmi
{
    // Connects to a namespace (e.g. L"ROOT\\CIMV2" / L"ROOT\\WMI"). Returns the
    // services (caller must Release) or nullptr. comInitialized is set true only
    // when this call actually did CoInitializeEx — the caller then owns the
    // matching CoUninitialize. RPC_E_CHANGED_MODE is treated as non-fatal.
    IWbemServices* Connect(const wchar_t* wmiNamespace, bool& comInitialized);

    std::wstring Trim(const wchar_t* s); // strip leading/trailing spaces

    // First matching row of a WQL query; returns object (caller Releases) or nullptr.
    IWbemClassObject* QueryFirst(IWbemServices* svc, const wchar_t* wql);

    bool         GetU32(IWbemClassObject* obj, const wchar_t* field, double& out); // VT_I4/VT_UI4
    std::wstring GetStr(IWbemClassObject* obj, const wchar_t* field);              // VT_BSTR, trimmed

    // Convenience: QueryFirst + GetStr + Release, for one-string-per-class reads.
    std::wstring QueryStr(IWbemServices* svc, const wchar_t* wql, const wchar_t* field);
}
