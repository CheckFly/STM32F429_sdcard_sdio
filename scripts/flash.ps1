[CmdletBinding()]
param(
    [string]$ElfPath = "",
    [string]$ProgrammerPath = "",
    [string]$Port = "SWD"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

if (-not $ElfPath) {
    $candidates = @(
        (Join-Path $root 'build\F429_sdcard_sdio_ecran.elf'),
        (Join-Path $root 'Debug\F429_sdcard_sdio_ecran.elf')
    )
    $ElfPath = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $ElfPath -or -not (Test-Path $ElfPath)) {
    throw "Firmware ELF not found. Build first with .\scripts\build.ps1"
}

if (-not $ProgrammerPath) {
    $defaultProgrammer = 'C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe'
    if (Test-Path $defaultProgrammer) {
        $ProgrammerPath = $defaultProgrammer
    }
    else {
        $command = Get-Command 'STM32_Programmer_CLI.exe' -ErrorAction SilentlyContinue
        if ($command) {
            $ProgrammerPath = $command.Source
        }
    }
}

if (-not $ProgrammerPath -or -not (Test-Path $ProgrammerPath)) {
    throw "STM32_Programmer_CLI.exe not found. Install STM32CubeProgrammer or pass -ProgrammerPath."
}

Write-Host "Programming: $ElfPath" -ForegroundColor Cyan
Write-Host "Programmer:  $ProgrammerPath" -ForegroundColor Cyan

& $ProgrammerPath '-c' "port=$Port" '-w' $ElfPath '-v' '-rst'
if ($LASTEXITCODE -ne 0) {
    throw "Flashing failed with exit code $LASTEXITCODE"
}

Write-Host "Flash OK" -ForegroundColor Green
