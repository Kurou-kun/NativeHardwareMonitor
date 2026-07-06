#include "Utils/WmiUtil.h"

#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")

namespace Wmi
{

IWbemServices* Connect(const wchar_t* wmiNamespace, bool& comInitialized)
{
    // RPC_E_CHANGED_MODE means the calling thread already has COM up in another
    // apartment (Rainmeter's own thread does) — WMI works fine either way; we
    // just must not CoUninitialize an apartment we didn't initialize.
    HRESULT coInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    comInitialized = SUCCEEDED(coInit);

    IWbemLocator* locator = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                 IID_IWbemLocator, (LPVOID*)&locator)) || !locator)
        return nullptr;

    IWbemServices* services = nullptr;
    HRESULT hr = locator->ConnectServer(_bstr_t(wmiNamespace), nullptr, nullptr, nullptr,
                                         0, nullptr, nullptr, &services);
    if (FAILED(hr) || !services)
    {
        locator->Release();
        return nullptr;
    }

    CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                       RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    locator->Release();
    return services;
}

std::wstring Trim(const wchar_t* s)
{
    if (!s) return L"";
    std::wstring v = s;
    size_t start = v.find_first_not_of(L' ');
    size_t end   = v.find_last_not_of(L' ');
    return start == std::wstring::npos ? L"" : v.substr(start, end - start + 1);
}

IWbemClassObject* QueryFirst(IWbemServices* svc, const wchar_t* wql)
{
    if (!svc)
        return nullptr;

    IEnumWbemClassObject* enumerator = nullptr;
    HRESULT hr = svc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(wql),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);

    if (FAILED(hr) || !enumerator)
        return nullptr;

    IWbemClassObject* obj = nullptr;
    ULONG returned = 0;
    enumerator->Next(WBEM_INFINITE, 1, &obj, &returned);
    enumerator->Release();

    return returned == 1 ? obj : nullptr;
}

bool GetU32(IWbemClassObject* obj, const wchar_t* field, double& out)
{
    VARIANT v; VariantInit(&v);
    bool ok = false;
    if (SUCCEEDED(obj->Get(field, 0, &v, nullptr, nullptr)))
    {
        if      (v.vt == VT_I4)  { out = v.lVal;  ok = true; }
        else if (v.vt == VT_UI4) { out = v.ulVal; ok = true; }
    }
    VariantClear(&v);
    return ok;
}

std::wstring GetStr(IWbemClassObject* obj, const wchar_t* field)
{
    VARIANT v; VariantInit(&v);
    std::wstring out;
    if (SUCCEEDED(obj->Get(field, 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR)
        out = Trim(v.bstrVal);
    VariantClear(&v);
    return out;
}

std::wstring QueryStr(IWbemServices* svc, const wchar_t* wql, const wchar_t* field)
{
    IWbemClassObject* obj = QueryFirst(svc, wql);
    if (!obj)
        return L"";
    std::wstring out = GetStr(obj, field);
    obj->Release();
    return out;
}

} // namespace Wmi
