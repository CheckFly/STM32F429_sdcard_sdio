[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [int]$DiskNumber,

    [int]$FirstPartitionGB = 4,

    [ValidateSet('FAT32', 'exFAT')]
    [string]$SecondFileSystem = 'exFAT',

    [switch]$Execute
)

$ErrorActionPreference = "Stop"

if ($FirstPartitionGB -lt 1) {
    throw "FirstPartitionGB must be at least 1."
}

$firstPartitionMB = $FirstPartitionGB * 1024
$scriptPath = Join-Path $env:TEMP "partition_sdcard_$DiskNumber.txt"

$diskpartScript = @"
select disk $DiskNumber
clean
create partition primary size=$firstPartitionMB
format fs=fat32 quick label=SDCARD
assign
create partition primary
format fs=$($SecondFileSystem.ToLower()) quick label=DATA
assign
exit
"@

Set-Content -Path $scriptPath -Value $diskpartScript -Encoding ASCII

Write-Host ""
Write-Host "WARNING: this operation erases ALL data on disk $DiskNumber." -ForegroundColor Red
Write-Host "Verify the target with: diskpart -> list disk" -ForegroundColor Yellow
Write-Host ""
Write-Host "Generated diskpart script: $scriptPath" -ForegroundColor Cyan
Write-Host "------------------------------------------------------------"
Write-Host $diskpartScript
Write-Host "------------------------------------------------------------"

if (-not $Execute) {
    Write-Host "Dry run only. Re-run with -Execute to apply after verifying the disk number." -ForegroundColor Yellow
    exit 0
}

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw "diskpart requires an elevated PowerShell session."
}

$confirmation = Read-Host "Type ERASE to erase disk $DiskNumber"
if ($confirmation -ne 'ERASE') {
    Write-Host "Cancelled." -ForegroundColor Yellow
    exit 1
}

& diskpart /s $scriptPath
if ($LASTEXITCODE -ne 0) {
    throw "diskpart failed with exit code $LASTEXITCODE"
}

Write-Host "Partitioning complete. Partition 1: ${FirstPartitionGB} GB FAT32 (SDCARD). Partition 2: remaining space $SecondFileSystem (DATA)." -ForegroundColor Green
