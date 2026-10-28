[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$Flash
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root

try {
    function Find-FirstTool {
        param([string]$Filter)
        Get-ChildItem -Path 'C:\ST' -Filter $Filter -Recurse -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1 -ExpandProperty FullName
    }

    $gcc = Find-FirstTool -Filter 'arm-none-eabi-gcc.exe'
    $make = Find-FirstTool -Filter 'make.exe'

    if (-not $gcc) {
        throw "arm-none-eabi-gcc.exe not found under C:\ST. Install STM32CubeIDE or add the toolchain to PATH."
    }
    if (-not $make) {
        throw "make.exe not found under C:\ST. Install STM32CubeIDE or add GNU make to PATH."
    }

    $gccBin = Split-Path -Parent $gcc
    $makeBin = Split-Path -Parent $make
    $env:PATH = "$gccBin;$makeBin;$env:PATH"

    Write-Host "Toolchain: $gccBin" -ForegroundColor Cyan
    Write-Host "Make:      $makeBin" -ForegroundColor Cyan

    if ($Clean) {
        Write-Host "Cleaning..." -ForegroundColor Yellow
        & make clean
        if ($LASTEXITCODE -ne 0) {
            throw "make clean failed with exit code $LASTEXITCODE"
        }
    }

    Write-Host "Building..." -ForegroundColor Green
    & make -j4
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }

    Write-Host "Build OK: build/F429_sdcard_sdio_ecran.elf" -ForegroundColor Green

    if ($Flash) {
        & (Join-Path $PSScriptRoot 'flash.ps1')
    }
}
finally {
    Pop-Location
}
