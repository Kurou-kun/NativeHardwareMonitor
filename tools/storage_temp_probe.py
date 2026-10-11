"""Ask every physical drive for its temperature, the way a driverless plugin would have to.

Issue #8. Windows exposes drive temperature through DeviceIoControl with
IOCTL_STORAGE_QUERY_PROPERTY / StorageDeviceTemperatureProperty, which would make it the
one temperature this plugin can read without a kernel driver. Before writing any C++,
this answers the questions that decide whether it is worth writing:

  - which drives answer at all (NVMe, SATA, USB enclosure)
  - whether a handle opened with NO access rights is enough, or it needs administrator
  - what a multi-sensor drive actually returns
  - how PhysicalDriveN maps onto the volumes the Storage category indexes today

    python tools/storage_temp_probe.py
"""
import ctypes as C
import ctypes.wintypes as W
import sys

GENERIC_READ = 0x80000000
FILE_SHARE_READ = 1
FILE_SHARE_WRITE = 2
OPEN_EXISTING = 3
INVALID_HANDLE = C.c_void_p(-1).value

IOCTL_STORAGE_QUERY_PROPERTY = 0x002D1400
PropertyStandardQuery = 0
StorageDeviceProperty = 0
StorageAdapterTemperatureProperty = 51
StorageDeviceTemperatureProperty = 52

k32 = C.WinDLL("kernel32", use_last_error=True)
# CreateFileW returns a HANDLE; left at the default c_int restype it comes back as -1 and
# compares unequal to INVALID_HANDLE_VALUE, so every absent drive looks like a live one.
k32.CreateFileW.restype = C.c_void_p
k32.CreateFileW.argtypes = [C.c_wchar_p, C.c_ulong, C.c_ulong, C.c_void_p,
                            C.c_ulong, C.c_ulong, C.c_void_p]


class STORAGE_PROPERTY_QUERY(C.Structure):
    _fields_ = [("PropertyId", C.c_ulong),
                ("QueryType", C.c_ulong),
                ("AdditionalParameters", C.c_ubyte * 1)]


class STORAGE_TEMPERATURE_INFO(C.Structure):
    _fields_ = [("Index", C.c_ushort),
                ("Temperature", C.c_short),
                ("OverThreshold", C.c_short),
                ("UnderThreshold", C.c_short),
                ("OverThresholdChanged", C.c_ubyte),
                ("UnderThresholdChanged", C.c_ubyte)]


class STORAGE_TEMPERATURE_DATA_DESCRIPTOR(C.Structure):
    _fields_ = [("Version", C.c_ulong),
                ("Size", C.c_ulong),
                ("CriticalTemperature", C.c_short),
                ("WarningTemperature", C.c_short),
                ("InfoCount", C.c_ushort),
                ("Reserved0", C.c_ubyte * 2),
                ("Reserved1", C.c_ulong * 2),
                ("TemperatureInfo", STORAGE_TEMPERATURE_INFO * 8)]


class STORAGE_DEVICE_DESCRIPTOR(C.Structure):
    _fields_ = [("Version", C.c_ulong), ("Size", C.c_ulong),
                ("DeviceType", C.c_ubyte), ("DeviceTypeModifier", C.c_ubyte),
                ("RemovableMedia", C.c_ubyte), ("CommandQueueing", C.c_ubyte),
                ("VendorIdOffset", C.c_ulong), ("ProductIdOffset", C.c_ulong),
                ("ProductRevisionOffset", C.c_ulong), ("SerialNumberOffset", C.c_ulong),
                ("BusType", C.c_ulong), ("RawPropertiesLength", C.c_ulong),
                ("RawDeviceProperties", C.c_ubyte * 1024)]


BUS = {1: "SCSI", 2: "ATAPI", 3: "ATA", 4: "1394", 5: "SSA", 6: "Fibre", 7: "USB",
       8: "RAID", 9: "iSCSI", 10: "SAS", 11: "SATA", 12: "SD", 13: "MMC",
       17: "NVMe", 18: "SCM", 19: "UFS"}


def open_drive(n, access):
    h = k32.CreateFileW(C.c_wchar_p(r"\\.\PhysicalDrive%d" % n), C.c_ulong(access),
                        C.c_ulong(FILE_SHARE_READ | FILE_SHARE_WRITE), None,
                        C.c_ulong(OPEN_EXISTING), C.c_ulong(0), None)
    return None if h == INVALID_HANDLE else h


def query(handle, prop_id, out_struct):
    q = STORAGE_PROPERTY_QUERY(prop_id, PropertyStandardQuery, (C.c_ubyte * 1)(0))
    out = out_struct()
    returned = C.c_ulong(0)
    ok = k32.DeviceIoControl(C.c_void_p(handle), C.c_ulong(IOCTL_STORAGE_QUERY_PROPERTY),
                             C.byref(q), C.c_ulong(C.sizeof(q)),
                             C.byref(out), C.c_ulong(C.sizeof(out)),
                             C.byref(returned), None)
    return (out if ok else None), (0 if ok else C.get_last_error())


def describe(handle):
    desc, err = query(handle, StorageDeviceProperty, STORAGE_DEVICE_DESCRIPTOR)
    if not desc:
        return "(device descriptor failed, err %d)" % err
    raw = C.string_at(C.byref(desc), min(desc.Size, C.sizeof(desc)))

    def at(off):
        if not off or off >= len(raw):
            return ""
        end = raw.index(b"\0", off)
        return raw[off:end].decode(errors="replace").strip()

    name = (at(desc.VendorIdOffset) + " " + at(desc.ProductIdOffset)).strip()
    return "%-40s bus=%s" % (name[:40], BUS.get(desc.BusType, desc.BusType))


def main():
    print("PhysicalDrive scan - temperature via IOCTL_STORAGE_QUERY_PROPERTY\n")
    found = False

    for n in range(16):
        h = open_drive(n, 0)          # no access rights at all - the interesting case
        access_used = "no-access"
        if h is None:
            h = open_drive(n, GENERIC_READ)
            access_used = "GENERIC_READ"
        if h is None:
            continue

        found = True
        print("PhysicalDrive%-2d %s   [opened with %s]" % (n, describe(h), access_used))

        for label, prop in (("device", StorageDeviceTemperatureProperty),
                            ("adapter", StorageAdapterTemperatureProperty)):
            t, err = query(h, prop, STORAGE_TEMPERATURE_DATA_DESCRIPTOR)
            if not t:
                print("    %-8s not supported (err %d)" % (label, err))
                continue
            print("    %-8s InfoCount=%d critical=%d warning=%d"
                  % (label, t.InfoCount, t.CriticalTemperature, t.WarningTemperature))
            for i in range(min(t.InfoCount, 8)):
                s = t.TemperatureInfo[i]
                print("        sensor %d: %d C   over=%d under=%d"
                      % (s.Index, s.Temperature, s.OverThreshold, s.UnderThreshold))

        k32.CloseHandle(C.c_void_p(h))
        print()

    if not found:
        print("no physical drive could be opened - try again as administrator")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
