"""Check Adl2Provider::EnumerateAdapters' filter against the real ADL driver.

Run on any machine with an AMD GPU and atiadlxx.dll present:
    python tools/adl2_enum_check.py
Fails if the filter the C++ uses would enumerate no AMD device.
"""
import ctypes as C
import sys

P = 256


class AdapterInfo(C.Structure):
    _fields_ = [("iSize", C.c_int), ("iAdapterIndex", C.c_int), ("strUDID", C.c_char * P),
                ("iBusNumber", C.c_int), ("iDeviceNumber", C.c_int), ("iFunctionNumber", C.c_int),
                ("iVendorID", C.c_int), ("strAdapterName", C.c_char * P),
                ("strDisplayName", C.c_char * P), ("iPresent", C.c_int), ("iExist", C.c_int),
                ("strDriverPath", C.c_char * P), ("strDriverPathExt", C.c_char * P),
                ("strPNPString", C.c_char * P), ("iOSDisplayIndex", C.c_int)]


def main():
    try:
        adl = C.WinDLL("atiadlxx.dll")
    except OSError:
        print("atiadlxx.dll not present - no AMD driver, nothing to check")
        return 0

    alloc_t = C.WINFUNCTYPE(C.c_void_p, C.c_int)
    held = []

    def alloc(n):
        b = C.create_string_buffer(n)
        held.append(b)
        return C.cast(b, C.c_void_p).value

    ctx = C.c_void_p()
    if adl.ADL2_Main_Control_Create(alloc_t(alloc), 1, C.byref(ctx)) != 0 or not ctx:
        print("ADL2_Main_Control_Create failed")
        return 1

    count = C.c_int()
    adl.ADL2_Adapter_NumberOfAdapters_Get(ctx, C.byref(count))
    info = (AdapterInfo * count.value)() if count.value > 0 else []
    if count.value > 0:
        adl.ADL2_Adapter_AdapterInfo_Get(ctx, info, C.sizeof(info))

    # Same filter as Adl2Provider::EnumerateAdapters
    buses, by_hex_only, active_kept = set(), 0, 0
    for a in info:
        if a.iVendorID not in (1002, 0x1002):
            continue
        act = C.c_int(-1)
        if adl.ADL2_Adapter_Active_Get(ctx, a.iAdapterIndex, C.byref(act)) == 0 and act.value:
            active_kept += 1
        if a.iVendorID == 0x1002:
            by_hex_only += 1
        if not a.iExist:
            continue
        buses.add(a.iBusNumber)
        print(f"  idx={a.iAdapterIndex:3} bus={a.iBusNumber:3} ven={a.iVendorID} "
              f"exist={a.iExist} present={a.iPresent} "
              f"active={act.value} {a.strAdapterName.decode(errors='replace')}")

    adl.ADL2_Main_Control_Destroy(ctx)

    print(f"adapters reported={count.value}  AMD devices after filter={len(buses)}  "
          f"(hex-0x1002 match would give {by_hex_only}, Active_Get gate would keep {active_kept})")
    assert len(buses) >= 1, "filter enumerated no AMD device - regression"
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
