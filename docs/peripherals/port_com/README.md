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

> **Decision required:** COM39 is assumed to be the ST-Link virtual COM port. The STM32F429I-DISCOVERY VCP is typically USART1 (PA9 TX / PA10 RX), but `PA10` is already used by `SD_CARD_DET`.

Options:

1. **USART1 TX only on PA9** — send logs, no receive. `PA10` stays the card-detect input.
2. **UART4 on PA0/PA1** — move COM39 to UART4 and keep PA9/PA10 free.
3. **External USB-UART adapter** — connect its RX to the chosen MCU TX pin.

Do not configure `PA10` as `USART1_RX` while it is used for card detection.

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
