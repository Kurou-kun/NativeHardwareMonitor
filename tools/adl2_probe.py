"""Ask the AMD driver what it will answer beyond Overdrive5.

Adl2Provider currently loads three Overdrive5 entry points (CurrentActivity,
Temperature, FanSpeed), so on an ADLX-less machine every other GPU metric reads -1 -
not because the driver refuses, but because nothing asks. This probes the calls worth
adding, read-only, and prints what each one returns:

    python tools/adl2_probe.py

  Overdrive_Caps        which Overdrive generation the adapter speaks (5/6/7/8)
  VRAMUsage_Get         VRAM in use, MB
  MemoryInfo2_Get       VRAM total / visible / invisible
  VideoBiosInfo_Get     VBIOS version string
  ODParameters_Get      OD5 clock ranges, i.e. MaxCoreClock
  New_QueryPMLogData    the modern sensor block: temperatures, power, voltages,
                        per-engine activity - the path GPU-Z and HWiNFO use
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


class MemoryInfo2(C.Structure):
    _fields_ = [("iMemorySize", C.c_longlong), ("strMemoryType", C.c_char * P),
                ("iMemoryBandwidth", C.c_longlong), ("iHyperMemorySize", C.c_longlong),
                ("iInvisibleMemorySize", C.c_longlong), ("iVisibleMemorySize", C.c_longlong)]


class BiosInfo(C.Structure):
    _fields_ = [("strPartNumber", C.c_char * P), ("strVersion", C.c_char * P),
                ("strDate", C.c_char * P)]


class ODParameterRange(C.Structure):
    _fields_ = [("iMin", C.c_int), ("iMax", C.c_int), ("iStep", C.c_int)]


class ODParameters(C.Structure):
    _fields_ = [("iSize", C.c_int), ("iNumberOfPerformanceLevels", C.c_int),
                ("iActivityReportingSupported", C.c_int), ("iDiscretePerformanceLevels", C.c_int),
                ("iReserved", C.c_int), ("sEngineClock", ODParameterRange),
                ("sMemoryClock", ODParameterRange), ("sVddc", ODParameterRange)]


PMLOG_MAX_SENSORS = 256


class SingleSensorData(C.Structure):
    _fields_ = [("supported", C.c_int), ("value", C.c_int)]


class PMLogDataOutput(C.Structure):
    _fields_ = [("size", C.c_int), ("sensors", SingleSensorData * PMLOG_MAX_SENSORS)]


# ADL_PMLOG_SENSORS, copied from adl_defines.h. The enum starts at 1 and the sensor id
# IS the array index, so an off-by-one here renames every reading - it did, once.
# Unlisted indices print as the raw number;
# a sensor that reports supported=1 at an index we have no name for is still a finding.
PMLOG_NAMES = {
    1: "CLK_GFXCLK", 2: "CLK_MEMCLK", 3: "CLK_SOCCLK", 4: "CLK_UVDCLK1", 5: "CLK_UVDCLK2",
    6: "CLK_VCECLK", 7: "CLK_VCNCLK", 8: "TEMPERATURE_EDGE", 9: "TEMPERATURE_MEM",
    10: "TEMPERATURE_VRVDDC", 11: "TEMPERATURE_VRMVDD", 12: "TEMPERATURE_LIQUID",
    13: "TEMPERATURE_PLX", 14: "FAN_RPM", 15: "FAN_PERCENTAGE", 16: "SOC_VOLTAGE",
    17: "SOC_POWER", 18: "SOC_CURRENT", 19: "INFO_ACTIVITY_GFX", 20: "INFO_ACTIVITY_MEM",
    21: "GFX_VOLTAGE", 22: "MEM_VOLTAGE", 23: "ASIC_POWER", 24: "TEMPERATURE_VRSOC",
    25: "TEMPERATURE_VRMVDD0", 26: "TEMPERATURE_VRMVDD1", 27: "TEMPERATURE_HOTSPOT",
    28: "TEMPERATURE_GFX", 29: "TEMPERATURE_SOC", 30: "GFX_POWER", 31: "GFX_CURRENT",
    32: "TEMPERATURE_CPU", 33: "CPU_POWER", 34: "CLK_CPUCLK", 35: "THROTTLER_STATUS",
    36: "CLK_VCN1CLK1", 37: "CLK_VCN1CLK2", 38: "SMART_POWERSHIFT_CPU",
    39: "SMART_POWERSHIFT_DGPU", 40: "BUS_SPEED", 41: "BUS_LANES",
    42: "TEMPERATURE_LIQUID0", 43: "TEMPERATURE_LIQUID1", 44: "CLK_FCLK",
}



def call(adl, name):
    try:
        return getattr(adl, name)
    except AttributeError:
        return None


def probe(adl, ctx, a):
    idx = a.iAdapterIndex
    print(f"\n=== adapter {idx}  bus {a.iBusNumber}  "
          f"{a.strAdapterName.decode(errors='replace')}")

    f = call(adl, "ADL2_Overdrive_Caps")
    if f:
        sup, en, ver = C.c_int(), C.c_int(), C.c_int()
        r = f(ctx, idx, C.byref(sup), C.byref(en), C.byref(ver))
        print(f"  Overdrive_Caps        r={r} supported={sup.value} enabled={en.value} "
              f"version={ver.value}")
    else:
        print("  Overdrive_Caps        not exported")

    f = call(adl, "ADL2_Adapter_VRAMUsage_Get")
    if f:
        mb = C.c_int()
        r = f(ctx, idx, C.byref(mb))
        print(f"  VRAMUsage_Get         r={r} {mb.value} MB")

    f = call(adl, "ADL2_Adapter_MemoryInfo2_Get")
    if f:
        mi = MemoryInfo2()
        r = f(ctx, idx, C.byref(mi))
        print(f"  MemoryInfo2_Get       r={r} total={mi.iMemorySize} "
              f"visible={mi.iVisibleMemorySize} invisible={mi.iInvisibleMemorySize} "
              f"type={mi.strMemoryType.decode(errors='replace').strip()}")

    f = call(adl, "ADL2_Adapter_VideoBiosInfo_Get")
    if f:
        bi = BiosInfo()
        r = f(ctx, idx, C.byref(bi))
        print(f"  VideoBiosInfo_Get     r={r} version="
              f"{bi.strVersion.decode(errors='replace').strip()!r} "
              f"part={bi.strPartNumber.decode(errors='replace').strip()!r}")

    f = call(adl, "ADL2_Overdrive5_ODParameters_Get")
    if f:
        od = ODParameters()
        od.iSize = C.sizeof(od)
        r = f(ctx, idx, C.byref(od))
        print(f"  ODParameters_Get      r={r} engine={od.sEngineClock.iMin}-"
              f"{od.sEngineClock.iMax} mem={od.sMemoryClock.iMin}-{od.sMemoryClock.iMax} "
              f"activityReporting={od.iActivityReportingSupported}")

    f = call(adl, "ADL2_New_QueryPMLogData_Get")
    if not f:
        print("  New_QueryPMLogData    not exported - driver predates PMLog")
        return
    out = PMLogDataOutput()
    r = f(ctx, idx, C.byref(out))
    live = [(i, s.value) for i, s in enumerate(out.sensors) if s.supported]
    print(f"  New_QueryPMLogData    r={r} size={out.size} sensors_supported={len(live)}")
    for i, v in live:
        print(f"      [{i:3}] {PMLOG_NAMES.get(i, '?'):<22} {v}")


def main():
    try:
        adl = C.WinDLL("atiadlxx.dll")
    except OSError:
        print("atiadlxx.dll not present - no AMD driver, nothing to probe")
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

    # One adapter per bus, same as Adl2Provider: ADL reports every output separately.
    seen = set()
    for a in info:
        if a.iVendorID not in (1002, 0x1002) or not a.iExist or a.iBusNumber in seen:
            continue
        seen.add(a.iBusNumber)
        probe(adl, ctx, a)

    adl.ADL2_Main_Control_Destroy(ctx)
    if not seen:
        print("no AMD adapter survived the filter")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
