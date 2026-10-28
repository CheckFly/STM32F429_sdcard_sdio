[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

& (Join-Path $root 'scripts\build.ps1') -Clean:$Clean -Flash
if ($LASTEXITCODE -ne 0) {
    throw "Build-and-flash failed with exit code $LASTEXITCODE"
}
