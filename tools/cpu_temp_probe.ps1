# Is there ANY driverless source of CPU temperature on this machine?
#
# Issue #6 asks for Category=CPU / Metric=Temperature. Every tool that shows a Ryzen or
# Core temperature (HWiNFO, LibreHardwareMonitor, Core Temp, Ryzen Master) reads it from
# model-specific registers or SMU mailboxes through a kernel driver - WinRing0 or the
# vendor's own. Loading one is the line this plugin exists not to cross, so the question
# is whether Windows or a vendor SDK will answer without it.
#
# Run it on any machine before concluding anything about #6. On the dev box
# (Ryzen 7 9700X, MSI PRO B840-P WIFI, 2026-10-12) every single check below came back
# empty - see the issue for the write-up. A laptop or an OEM desktop may well differ:
# those usually DO implement an ACPI thermal zone, which is what check 1 and 2 read.
#
# The AMD SDK side is checked separately by tools/adl2_probe.py: PMLog carries
# ADL_PMLOG_TEMPERATURE_CPU (sensor 32), CPU_POWER (33) and CLK_CPUCLK (34), and an APU
# whose driver marks those supported would be a genuine driverless CPU temperature.
# Raphael's does not - it offers TEMPERATURE_GFX and TEMPERATURE_SOC only.

'=== 1. ACPI thermal zone (root/wmi MSAcpi_ThermalZoneTemperature) - tenths of a Kelvin'
try {
    Get-CimInstance -Namespace root/wmi -ClassName MSAcpi_ThermalZoneTemperature -ErrorAction Stop |
        Select-Object InstanceName,
                      @{n = 'DegC'; e = { [math]::Round($_.CurrentTemperature / 10 - 273.15, 1) } },
                      CriticalTripPoint |
        Format-Table -AutoSize
} catch {
    "  unavailable: $($_.Exception.Message)"
}

'=== 2. ACPI thermal zone devices Windows actually enumerated'
$tz = @(Get-PnpDevice -ErrorAction SilentlyContinue | Where-Object InstanceId -match 'THERMAL|ACPI.TZ')
if ($tz) { $tz | Select-Object Status, FriendlyName, InstanceId | Format-Table -AutoSize }
else     { '  none - the firmware exposes no _TZ, so check 1 and 3 cannot work here' }

'=== 3. PDH Thermal Zone Information counter set'
$sets = @(Get-Counter -ListSet * -ErrorAction SilentlyContinue | Where-Object CounterSetName -match 'Thermal|Temperature')
if ($sets) { $sets.CounterSetName } else { '  counter set absent' }

'=== 4. Win32_TemperatureProbe (SMBIOS type 28; almost never populated)'
"  instances: $(@(Get-CimInstance Win32_TemperatureProbe -ErrorAction SilentlyContinue).Count)"

'=== 5. root/Hardware CIM sensors (the SMASH profile - servers with a BMC populate this)'
"  CIM_NumericSensor: $(@(Get-CimInstance -Namespace root/Hardware -ClassName CIM_NumericSensor -ErrorAction SilentlyContinue).Count)"
"  CIM_Sensor:        $(@(Get-CimInstance -Namespace root/Hardware -ClassName CIM_Sensor -ErrorAction SilentlyContinue).Count)"

'=== 6. A board-vendor WMI sensor class, if the vendor ships one'
Get-CimInstance Win32_BaseBoard | Select-Object Manufacturer, Product | Format-List
$cls = @(Get-CimClass -Namespace root/wmi -ErrorAction SilentlyContinue |
         Where-Object CimClassName -match 'Temp|Thermal' | Select-Object -ExpandProperty CimClassName)
if ($cls) { "  root/wmi classes: $($cls -join ', ')" } else { '  none' }

'=== 7. A third-party sensor service already running (NOT a route this plugin may take -'
'       listed only so a stray reading here is not mistaken for a driverless one)'
$ns = @(Get-CimInstance -Namespace root -ClassName __NAMESPACE -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty Name |
        Where-Object { $_ -match 'OpenHardware|Libre|HWiNFO|Afterburner' })
if ($ns) { "  present: $($ns -join ', ')" } else { '  none installed' }
