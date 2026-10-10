<#
    Builds the .rmskin package.

        msbuild NativeHardwareMonitor.vcxproj /p:Configuration=Release /p:Platform=x64
        msbuild NativeHardwareMonitor.vcxproj /p:Configuration=Release /p:Platform=Win32
        .\tools\build-package.ps1

    A .rmskin is a plain zip (RMSKIN.ini, Skins\, Plugins\32bit\, Plugins\64bit\)
    with a 16-byte footer appended: the zip length as a little-endian uint64,
    then 0x00, "RMSKIN", 0x00. Rainmeter reads that footer to find the archive,
    so a zip without it is not installable.

    The plugin DLL that ends up in @Vault is the one in Plugins\, which is why
    the package has to carry both architectures - the installer picks per host
    and refuses a package that is missing one.
#>
[CmdletBinding()]
param(
    [string]$Configuration = 'Release',
    [string]$OutputDir     = 'dist'
)

$ErrorActionPreference = 'Stop'

$root  = Split-Path $PSScriptRoot -Parent
$pkg   = Join-Path $root 'package'
$ini   = Join-Path $pkg  'RMSKIN.ini'
$dll64 = Join-Path $root "x64\$Configuration\NativeHardwareMonitor.dll"
$dll32 = Join-Path $root "$Configuration\NativeHardwareMonitor.dll"

foreach ($f in @($ini, $dll64, $dll32)) {
    if (-not (Test-Path $f)) { throw "missing $f - build both platforms first" }
}

# The installer compares the FileVersion RESOURCE, not the file name, when it
# decides whether to replace an installed plugin. A package whose DLLs disagree
# with its own version is the 1.0.1 bug all over again, so fail loudly here.
$version = ([regex]::Match((Get-Content $ini -Raw), '(?m)^\s*Version\s*=\s*(.+?)\s*$')).Groups[1].Value
if (-not $version) { throw "no Version= in $ini" }

foreach ($pair in @(@{ Path = $dll64; Arch = '(64-bit)' }, @{ Path = $dll32; Arch = '(32-bit)' })) {
    $info = (Get-Item $pair.Path).VersionInfo
    if ($info.FileVersion -notlike "$version*") {
        throw "$($pair.Path) is FileVersion $($info.FileVersion), expected $version - bump NativeHardwareMonitor.rc"
    }
    if ($info.ProductVersion -notlike "*$($pair.Arch)*") {
        throw "$($pair.Path) reports ProductVersion $($info.ProductVersion), expected $($pair.Arch) - check the ResourceCompile _WIN64 define"
    }
}

Add-Type -AssemblyName System.IO.Compression.FileSystem

$out = Join-Path $root $OutputDir
New-Item -ItemType Directory -Force -Path $out | Out-Null
$rmskin = Join-Path $out "NativeHardwareMonitor_$version.rmskin"
$tmpZip = [System.IO.Path]::GetTempFileName()
Remove-Item $tmpZip -Force   # ZipFile::Open wants to create it itself

# Collect (source file -> entry name). Entry names always use forward slashes;
# Rainmeter will not find "Skins\x" written with backslashes.
$entries = [ordered]@{ $ini = 'RMSKIN.ini' }
foreach ($f in Get-ChildItem (Join-Path $pkg 'Skins') -Recurse -File) {
    $rel = $f.FullName.Substring($pkg.Length + 1).Replace('\', '/')
    $entries[$f.FullName] = $rel
}
$entries[$dll32] = 'Plugins/32bit/NativeHardwareMonitor.dll'
$entries[$dll64] = 'Plugins/64bit/NativeHardwareMonitor.dll'

$zip = [System.IO.Compression.ZipFile]::Open($tmpZip, 'Create')
try {
    foreach ($src in $entries.Keys) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $zip, $src, $entries[$src], [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
}
finally { $zip.Dispose() }

$bytes  = [System.IO.File]::ReadAllBytes($tmpZip)
$footer = [byte[]]::new(16)
[System.BitConverter]::GetBytes([uint64]$bytes.Length).CopyTo($footer, 0)
$footer[8] = 0x00
[System.Text.Encoding]::ASCII.GetBytes('RMSKIN').CopyTo($footer, 9)
$footer[15] = 0x00

[System.IO.File]::WriteAllBytes($rmskin, ($bytes + $footer))
Remove-Item $tmpZip -Force

"{0}  ({1:N0} bytes, {2} entries, version {3})" -f $rmskin, (Get-Item $rmskin).Length, $entries.Count, $version
