[CmdletBinding()]
param(
    [string]$Port = "COM39",
    [int]$Baud = 115200,
    [int]$DurationSeconds = 0,
    [string]$LogPath = "logs\sd-monitor.log"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

if (-not [System.IO.Path]::IsPathRooted($LogPath)) {
    $LogPath = Join-Path $root $LogPath
}

& (Join-Path $root 'scripts\open_com.ps1') `
    -Port $Port `
    -Baud $Baud `
    -DurationSeconds $DurationSeconds `
    -LogPath $LogPath `
    -Filter 'SD:'
