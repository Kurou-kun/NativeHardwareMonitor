// x86 stub for the Rainmeter package installer.
//
// The real plugin is x64-only: the vendor libraries are all LoadLibrary'd at runtime,
// but NVIDIA ships no 32-bit nvml.dll, so an x86 build could not read a modern NVIDIA
// GPU at all. A .rmskin
// must still carry a 32-bit DLL or the installer rejects it and 32-bit Rainmeter
// can't load the measure at all. This stub exports the five Rainmeter entry points
// and reports the unsupported sentinel (-1) for every metric — no hardware access,
// no vendor libs, no Rainmeter.lib dependency.
//
// ponytail: intentionally does nothing. Build the real functionality here only if a
// 32-bit vendor SDK ever ships and someone actually runs 32-bit Rainmeter.

#include <windows.h>

#define PLUGIN_EXPORT EXTERN_C __declspec(dllexport)

PLUGIN_EXPORT void Initialize(void** data, void* /*rm*/) { *data = nullptr; }
PLUGIN_EXPORT void Reload(void* /*data*/, void* /*rm*/, double* /*maxValue*/) {}
PLUGIN_EXPORT double Update(void* /*data*/) { return -1.0; } // -1 = hardware unsupported
PLUGIN_EXPORT LPCWSTR GetString(void* /*data*/) { return nullptr; }
PLUGIN_EXPORT void Finalize(void* /*data*/) {}
