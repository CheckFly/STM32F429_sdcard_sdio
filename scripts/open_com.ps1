[CmdletBinding()]
param(
    [string]$Port = "COM39",
    [int]$Baud = 115200,
    [string]$LogPath = "",
    [int]$DurationSeconds = 0,
    [string]$Filter = ""
)

$ErrorActionPreference = "Stop"

if ($LogPath) {
    $logDir = Split-Path -Parent $LogPath
    if ($logDir -and -not (Test-Path $logDir)) {
        New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    }
}

$serial = New-Object System.IO.Ports.SerialPort $Port, $Baud, ([System.IO.Ports.Parity]::None), 8, ([System.IO.Ports.StopBits]::One)
$serial.ReadTimeout = 500
$serial.NewLine = "`r`n"

try {
    $serial.Open()
    Write-Host "Listening on $Port at $Baud baud. Ctrl+C to stop." -ForegroundColor Cyan
    $start = Get-Date

    while ($true) {
        if ($DurationSeconds -gt 0 -and ((Get-Date) - $start).TotalSeconds -ge $DurationSeconds) {
            break
        }

        try {
            $line = $serial.ReadLine()
            if ($Filter -and ($line -notmatch $Filter)) {
                continue
            }
            $entry = "{0:yyyy-MM-dd HH:mm:ss.fff} {1}" -f (Get-Date), $line
            Write-Host $entry
            if ($LogPath) {
                Add-Content -Path $LogPath -Value $entry
            }
        }
        catch [System.TimeoutException] {
            # No complete line yet; keep polling.
        }
    }
}
finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
    $serial.Dispose()
}
