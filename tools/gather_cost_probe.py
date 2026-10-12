"""What does one gather actually cost, per call?

Issue #7 asks for metric-level gathering: read only the metrics a skin displays. The win
exists only where a provider makes a separate driver call per metric - so before building
the machinery to carry a wanted-set down to every provider, measure how much there is to
save. A provider whose cheapest call returns a whole telemetry block (ADL2 PMLog, IGCL
telemetry) cannot be subset at all, and the saving there is exactly zero.

This times the calls NvmlProvider::GatherSnapshot makes, one at a time, straight through
nvml.dll - no plugin build, no instrumentation. PDH counter reads (CPU, Network, Storage)
are timed the same way through pdh.dll.

Run: python tools/gather_cost_probe.py
"""

import ctypes as C
import time

NVML_PATHS = [
    r"C:\Program Files\NVIDIA Corporation\NVSMI\nvml.dll",
    r"C:\Windows\System32\nvml.dll",
]
SAMPLES = 200


def load_nvml():
    for p in NVML_PATHS:
        try:
            return C.CDLL(p)
        except OSError:
            continue
    return None


def time_call(fn, *args, samples=SAMPLES):
    """Median microseconds per call - median, not mean: one scheduler hiccup should not
    decide whether a feature is worth building."""
    times = []
    for _ in range(samples):
        t0 = time.perf_counter_ns()
        fn(*args)
        times.append((time.perf_counter_ns() - t0) / 1000.0)
    times.sort()
    return times[len(times) // 2]


def probe_nvml():
    nvml = load_nvml()
    if not nvml:
        print("nvml.dll not found - skipping the NVIDIA half\n")
        return 0.0

    if nvml.nvmlInit_v2() != 0:
        print("nvmlInit failed\n")
        return 0.0

    dev = C.c_void_p()
    if nvml.nvmlDeviceGetHandleByIndex_v2(0, C.byref(dev)) != 0:
        print("no NVML device 0\n")
        return 0.0

    u32, u64 = C.c_uint(), C.c_ulonglong()

    class Util(C.Structure):
        _fields_ = [("gpu", C.c_uint), ("memory", C.c_uint)]

    class Mem(C.Structure):
        _fields_ = [("total", C.c_ulonglong), ("free", C.c_ulonglong), ("used", C.c_ulonglong)]

    util, mem = Util(), Mem()

    calls = [
        ("GetTemperature",        lambda: nvml.nvmlDeviceGetTemperature(dev, 0, C.byref(u32))),
        ("GetClockInfo (gfx)",    lambda: nvml.nvmlDeviceGetClockInfo(dev, 0, C.byref(u32))),
        ("GetClockInfo (mem)",    lambda: nvml.nvmlDeviceGetClockInfo(dev, 2, C.byref(u32))),
        ("GetMaxClockInfo",       lambda: nvml.nvmlDeviceGetMaxClockInfo(dev, 0, C.byref(u32))),
        ("GetUtilizationRates",   lambda: nvml.nvmlDeviceGetUtilizationRates(dev, C.byref(util))),
        ("GetMemoryInfo",         lambda: nvml.nvmlDeviceGetMemoryInfo(dev, C.byref(mem))),
        ("GetFanSpeed",           lambda: nvml.nvmlDeviceGetFanSpeed(dev, C.byref(u32))),
        ("GetPowerUsage",         lambda: nvml.nvmlDeviceGetPowerUsage(dev, C.byref(u32))),
        ("GetPowerManagementLimit", lambda: nvml.nvmlDeviceGetPowerManagementLimit(dev, C.byref(u32))),
        ("GetPerformanceState",   lambda: nvml.nvmlDeviceGetPerformanceState(dev, C.byref(u32))),
        ("GetCurrPcieLinkGeneration", lambda: nvml.nvmlDeviceGetCurrPcieLinkGeneration(dev, C.byref(u32))),
        ("GetCurrPcieLinkWidth",  lambda: nvml.nvmlDeviceGetCurrPcieLinkWidth(dev, C.byref(u32))),
        ("GetEncoderUtilization", lambda: nvml.nvmlDeviceGetEncoderUtilization(dev, C.byref(u32), C.byref(u32))),
        ("GetDecoderUtilization", lambda: nvml.nvmlDeviceGetDecoderUtilization(dev, C.byref(u32), C.byref(u32))),
        ("GetCurrentClocksThrottleReasons", lambda: nvml.nvmlDeviceGetCurrentClocksThrottleReasons(dev, C.byref(u64))),
    ]

    print("NVML - one call per metric, so this is where filtering could pay")
    print(f"{'call':38} {'median us':>10}")
    total = 0.0
    for name, fn in calls:
        us = time_call(fn)
        total += us
        print(f"  {name:36} {us:10.1f}")
    print(f"  {'TOTAL per device per gather':36} {total:10.1f} us")
    nvml.nvmlShutdown()
    return total


def probe_pdh():
    pdh = C.WinDLL("pdh.dll")
    query = C.c_void_p()
    if pdh.PdhOpenQueryW(None, 0, C.byref(query)) != 0:
        print("\nPdhOpenQuery failed")
        return 0.0

    counters = {
        r"\Processor Information(*)\% Processor Utility": None,
        r"\PhysicalDisk(*)\Disk Read Bytes/sec": None,
        r"\Network Interface(*)\Bytes Received/sec": None,
    }
    for path in list(counters):
        h = C.c_void_p()
        if pdh.PdhAddEnglishCounterW(query, path, 0, C.byref(h)) == 0:
            counters[path] = h

    pdh.PdhCollectQueryData(query)
    time.sleep(1.0)
    pdh.PdhCollectQueryData(query)

    print("\nPDH - one array read per counter, each returning every instance")
    print(f"{'counter':38} {'median us':>10}")
    total = 0.0

    def read(h):
        size, count = C.c_ulong(0), C.c_ulong(0)
        pdh.PdhGetFormattedCounterArrayW(h, 0x00000200, C.byref(size), C.byref(count), None)
        buf = C.create_string_buffer(size.value) if size.value else None
        if buf:
            pdh.PdhGetFormattedCounterArrayW(h, 0x00000200, C.byref(size), C.byref(count), buf)

    for path, h in counters.items():
        if not h:
            print(f"  {path[:36]:36} {'unavailable':>10}")
            continue
        us = time_call(read, h, samples=50)
        total += us
        print(f"  {path[:36]:36} {us:10.1f}")

    us = time_call(lambda: pdh.PdhCollectQueryData(query), samples=50)
    print(f"  {'PdhCollectQueryData (once per tick)':36} {us:10.1f}")
    pdh.PdhCloseQuery(query)
    return total + us


if __name__ == "__main__":
    nvml_us = probe_nvml()
    pdh_us = probe_pdh()
    print("\n--- verdict material ---")
    print(f"NVML full gather: {nvml_us:.0f} us/device   PDH reads: {pdh_us:.0f} us")
    print(f"At 1 Hz that is {(nvml_us + pdh_us) / 10000:.3f}% of one core for both.")
    print("ADL2 PMLog and IGCL telemetry return a whole block in one call - nothing to subset.")
