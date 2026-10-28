# COM port — COM39

## Settings

| Item | Value |
|------|-------|
| Port | `COM39` |
| Baud rate | `115200` |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |

## UART mapping

COM39 is the ST-Link Virtual COM Port:

```text
Name: STMicroelectronics STLink Virtual COM Port (COM39)
USB VID/PID: 0483:374B
```

The VCP uses **USART1 TX on PA9**. This was verified: the firmware emits `SD:REMOVED` on COM39 at 115200 8N1.

`PA10` is `USART1_RX` on the MCU, but it is reserved for `SD_CARD_DET`. The current firmware therefore uses **USART1 TX only**. Do not enable USART1 RX while `PA10` is used for card detection.

Options if bidirectional serial is required:

1. Move `SD_CARD_DET` to another free GPIO and enable USART1 RX.
2. Use UART4 on PA0/PA1 and rewire the adapter.
3. Use an external USB-UART adapter on the chosen TX/RX pins.

The ST community notes that some boards require solder bridges SB11/SB15 to connect USART1 to the ST-Link VCP (UM1670 chapter 6.3.3).

## Firmware messages

| Message | Meaning |
|---------|---------|
| `SD:PRESENT` | Card-detect input became active |
| `SD:REMOVED` | Card-detect input became inactive |
| `SD:MOUNT_OK` | FatFs mount succeeded |
| `SD:MOUNT_FAIL` | FatFs mount failed |
| `SD:PART1_OK` | Partition 1 (4 GB) mounted |
| `SD:PART2_OK` | Partition 2 (remaining space) mounted |

Each message is terminated by `\r\n`.

## Monitor

```powershell
.\scripts\open_com.ps1 -Port COM39 -Baud 115200
```

Filter only SD messages:

```powershell
.\scripts\monitor_sd.ps1 -Port COM39
```

Python:

```powershell
python -m serial.tools.miniterm COM39 115200
```

## Automated test

```powershell
python .\tests\test_com39.py --port COM39 --baud 115200 --duration 20 `
  --expect SD:PRESENT --expect SD:REMOVED
```

## Troubleshooting

| Symptom | Check |
|---------|-------|
| `Access is denied` when opening COM39 | Another terminal/CubeIDE console is using it |
| No output | UART pin mapping, TX pin, baud rate, ground |
| Garbage characters | Baud rate mismatch or wrong clock |
| Only RX works / no TX | TX pin not configured as AF7/AF8 |
| COM39 disappears | USB cable/ST-Link connection |

## Safety

- Never drive `PA10` as an output while it is connected to the ST-Link RX line.
- Confirm the UART instance before enabling the peripheral.
