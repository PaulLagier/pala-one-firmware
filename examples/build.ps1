# Pala One app builder for Windows.
#
# Builds a single example app without requiring `make` or Git Bash. Uses the
# xtensa-esp32s3 toolchain installed by Arduino IDE plus the system Python.
#
# Usage (from the repo root or anywhere):
#   .\examples\build.ps1 examples\reading_streak
#   .\examples\build.ps1 examples\wordle
#   .\examples\build.ps1 examples\click_counter
#   .\examples\build.ps1 examples\palagotchi -Clean
#
# Equivalent to running `make` against the Makefile, but pure PowerShell.

[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [string]$AppDir,

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

# --- Locate toolchain --------------------------------------------------------
$tcBase = Join-Path $env:LOCALAPPDATA 'Arduino15\packages\esp32\tools\esp-x32'
if (-not (Test-Path $tcBase)) {
    throw "ESP32 toolchain not found at $tcBase. Install the ESP32 board package via Arduino IDE first."
}
$tcVer = (Get-ChildItem $tcBase -Directory | Sort-Object Name -Descending | Select-Object -First 1).Name
$bin = Join-Path $tcBase "$tcVer\bin"

$cc      = Join-Path $bin 'xtensa-esp32s3-elf-gcc.exe'
$objcopy = Join-Path $bin 'xtensa-esp32s3-elf-objcopy.exe'
$nm      = Join-Path $bin 'xtensa-esp32s3-elf-nm.exe'
$readelf = Join-Path $bin 'xtensa-esp32s3-elf-readelf.exe'

foreach ($t in @($cc, $objcopy, $nm, $readelf)) {
    if (-not (Test-Path $t)) { throw "Missing toolchain binary: $t" }
}

# --- Locate Python -----------------------------------------------------------
$python = (Get-Command python -ErrorAction SilentlyContinue)
if (-not $python) { $python = Get-Command python3 -ErrorAction SilentlyContinue }
if (-not $python) { throw "Python 3 not found. Install from python.org and ensure `python` is on PATH." }
$python = $python.Source

# --- Resolve app paths -------------------------------------------------------
$AppDir = (Resolve-Path $AppDir).Path
$appName = Split-Path $AppDir -Leaf
$src    = Join-Path $AppDir 'app.c'
$ld     = Join-Path $AppDir 'pala_app.ld'
$elf    = Join-Path $AppDir "$appName.elf"
$binOut = Join-Path $AppDir "$appName.bin"

if (-not (Test-Path $src)) { throw "No app.c in $AppDir" }
if (-not (Test-Path $ld))  { throw "No pala_app.ld in $AppDir" }

if ($Clean) {
    if (Test-Path $elf)    { Remove-Item $elf -Force }
    if (Test-Path $binOut) { Remove-Item $binOut -Force }
    Write-Host "Cleaned $appName."
    return
}

# --- Resolve firmware header include path ------------------------------------
# Script lives in examples/; headers are at <repo>/Pala_One_2_1.
$headers = (Resolve-Path (Join-Path $PSScriptRoot '..\Pala_One_2_1')).Path
if (-not (Test-Path (Join-Path $headers 'pala_api.h'))) {
    throw "Cannot find pala_api.h under $headers"
}

# --- Compile -----------------------------------------------------------------
$cflags = @(
    '-fPIC', '-mlongcalls', '-Os', '-fno-jump-tables',
    '-nostdlib', '-nodefaultlibs',
    "-I$headers"
)

Write-Host "[1/3] Compiling $appName ..."
& $cc @cflags "-Wl,-T,$ld" '-Wl,-shared' '-o' $elf $src
if ($LASTEXITCODE -ne 0) { throw "gcc failed (exit $LASTEXITCODE)" }

# --- Strip to raw binary -----------------------------------------------------
Write-Host "[2/3] Stripping ELF to raw binary ..."
& $objcopy '-O' binary $elf $binOut
if ($LASTEXITCODE -ne 0) { throw "objcopy failed (exit $LASTEXITCODE)" }

# --- Find app_main entry offset via nm ---------------------------------------
$nmLines = & $nm $elf
if ($LASTEXITCODE -ne 0) { throw "nm failed (exit $LASTEXITCODE)" }
$entryMatch = $nmLines | Where-Object { $_ -match '^([0-9a-fA-F]+)\s+[Tt]\s+app_main$' } | Select-Object -First 1
if (-not $entryMatch) { throw "app_main symbol not found in $elf" }
$entryOff = [Convert]::ToUInt32(($entryMatch -split '\s+')[0], 16)

# --- Patch header (entry offset + relocation table) via Python ---------------
# Mirrors the Python snippet in the Makefile: read readelf -r, collect
# R_XTENSA_RELATIVE offsets, append them as a table, and write the three
# patched header fields (entry_offset @40, reloc_offset @44, reloc_count @48).
Write-Host "[3/3] Patching header (entry + relocation table) ..."
$pyScript = @'
import struct, subprocess, sys
readelf, elf_in, bin_path, entry_off = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
out = subprocess.check_output([readelf, '-r', elf_in], text=True)
relocs = [int(ln.split()[0], 16) for ln in out.splitlines() if 'R_XTENSA_RELATIVE' in ln]
d = bytearray(open(bin_path, 'rb').read())
struct.pack_into('<I', d, 40, entry_off)
reloc_off = len(d) if relocs else 0
d.extend(struct.pack('<' + 'I' * len(relocs), *relocs))
struct.pack_into('<I', d, 44, reloc_off)
struct.pack_into('<I', d, 48, len(relocs))
open(bin_path, 'wb').write(d)
print(f'relocations: {len(relocs)} ({relocs})')
'@

& $python -c $pyScript $readelf $elf $binOut $entryOff
if ($LASTEXITCODE -ne 0) { throw "Python patch step failed (exit $LASTEXITCODE)" }

$size = (Get-Item $binOut).Length
Write-Host ""
Write-Host "Built $binOut" -ForegroundColor Green
Write-Host ("  size:  {0} bytes" -f $size)
Write-Host ("  entry: 0x{0:x}" -f $entryOff)
