"""Ask NVAPI for the clocks NvapiProvider reads, and say whether they match NVML.

NvapiProvider fills CoreClock/MemoryClock from NvAPI_GPU_GetAllClockFrequencies, but
only when NVML did not supply them first - so on a machine where NVML works the NVAPI
path is never exercised and a mistake in it is invisible. This calls it exactly the way
the plugin does and prints both sources side by side:

    python tools/nvapi_clock_check.py

NVML's numbers come from nvidia-smi, which ships with the driver. A disagreement here is
a disagreement between the two NVIDIA APIs on this hardware, not necessarily a bug - but
a *scaling* disagreement (a clean 10x, 100x, 1000x) would be ours.
"""
import ctypes as C
import subprocess
import sys

NVAPI_MAX_GPU_PUBLIC_CLOCKS = 32
CLOCK_GRAPHICS = 0
CLOCK_MEMORY = 4

ID_Initialize = 0x0150E828
ID_Unload = 0xD22BDD7E
ID_EnumPhysicalGPUs = 0xE5AC921F
ID_GetAllClockFrequencies = 0xDCB616C3
ID_GetFullName = 0xCEEE8E9F

CURRENT_FREQ, BASE_CLOCK, BOOST_CLOCK = 0, 1, 2
TYPE_NAMES = {CURRENT_FREQ: "current", BASE_CLOCK: "base", BOOST_CLOCK: "boost"}


class Domain(C.Structure):
    # NvU32 bIsPresent:1; NvU32 reserved:31;  then NvU32 frequency (kHz)
    _fields_ = [("flags", C.c_uint32), ("frequency", C.c_uint32)]

    @property
    def present(self):
        return bool(self.flags & 1)


class ClockFrequencies(C.Structure):
    _fields_ = [("version", C.c_uint32),
                ("bits", C.c_uint32),          # ClockType:4, reserved:20, reserved1:8
                ("domain", Domain * NVAPI_MAX_GPU_PUBLIC_CLOCKS)]


def version_of(struct_type, ver):
    return C.sizeof(struct_type) | (ver << 16)


def smi_clocks():
    try:
        out = subprocess.run(
            ["nvidia-smi", "--query-gpu=clocks.gr,clocks.mem", "--format=csv,noheader,nounits"],
            capture_output=True, text=True, timeout=20).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return []
    rows = []
    for line in out.splitlines():
        parts = [p.strip() for p in line.split(",")]
        if len(parts) == 2:
            rows.append((parts[0], parts[1]))
    return rows


def main():
    try:
        nvapi = C.WinDLL("nvapi64.dll")
    except OSError:
        print("nvapi64.dll not present - no NVIDIA driver, nothing to check")
        return 0

    qi = nvapi.nvapi_QueryInterface
    qi.restype = C.c_void_p
    qi.argtypes = [C.c_uint32]

    def fn(id_, restype, *argtypes):
        addr = qi(id_)
        if not addr:
            return None
        proto = C.CFUNCTYPE(restype, *argtypes)
        return proto(addr)

    init = fn(ID_Initialize, C.c_int)
    if not init or init() != 0:
        print("NvAPI_Initialize failed")
        return 1

    enum = fn(ID_EnumPhysicalGPUs, C.c_int, C.POINTER(C.c_void_p), C.POINTER(C.c_uint32))
    gpus = (C.c_void_p * 64)()
    count = C.c_uint32(0)
    if enum(C.cast(gpus, C.POINTER(C.c_void_p)), C.byref(count)) != 0 or count.value == 0:
        print("NvAPI_EnumPhysicalGPUs found nothing")
        return 1

    get_name = fn(ID_GetFullName, C.c_int, C.c_void_p, C.c_char_p)
    get_clocks = fn(ID_GetAllClockFrequencies, C.c_int, C.c_void_p, C.POINTER(ClockFrequencies))
    if not get_clocks:
        print("NvAPI_GPU_GetAllClockFrequencies not exported")
        return 1

    smi = smi_clocks()

    for i in range(count.value):
        name = C.create_string_buffer(64)
        if get_name:
            get_name(gpus[i], name)
        print("\n=== GPU %d  %s" % (i, name.value.decode(errors="replace")))

        for clock_type in (CURRENT_FREQ, BASE_CLOCK, BOOST_CLOCK):
            clk = ClockFrequencies()
            # VER_3 is what the plugin sends: sizeof(V2) with a version of 3.
            clk.version = version_of(ClockFrequencies, 3)
            clk.bits = clock_type & 0xF
            r = get_clocks(gpus[i], C.byref(clk))
            if r != 0:
                print("  %-8s NvAPI error %d" % (TYPE_NAMES[clock_type], r))
                continue

            g = clk.domain[CLOCK_GRAPHICS]
            m = clk.domain[CLOCK_MEMORY]
            print("  %-8s graphics %s  memory %s" % (
                TYPE_NAMES[clock_type],
                ("%d kHz = %.1f MHz" % (g.frequency, g.frequency / 1000.0)) if g.present else "absent",
                ("%d kHz = %.1f MHz" % (m.frequency, m.frequency / 1000.0)) if m.present else "absent"))

        if i < len(smi):
            print("  nvidia-smi (NVML)  graphics %s MHz  memory %s MHz" % smi[i])

    unload = fn(ID_Unload, C.c_int)
    if unload:
        unload()
    return 0


if __name__ == "__main__":
    sys.exit(main())
