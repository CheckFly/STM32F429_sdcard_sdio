---
name: stm32-flash
description: Flash the STM32F429I-DISCOVERY board with the built firmware, open COM39, and validate SD card insertion/removal messages. Use for programming, serial monitoring, and hardware smoke tests.
license: MIT
compatibility: opencode
metadata:
  board: 32F429IDISCOVERY
  com_port: COM39
  baudrate: 115200
---

# Flash and serial test workflow

## What I do

- Program the board through ST-Link/SWD.
- Open `COM39` at `115200 8N1`.
- Verify the SD card presence protocol.
- Produce a repeatable smoke test after each build.

## When to use me

Use me after a successful build and whenever the SD card detection or the serial output changes.

## Flash the ELF

```powershell
.\scripts\flash.ps1
```

Equivalent command:

```powershell
& 'C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe' `
  -c port=SWD `
  -w 'build\F429_sdcard_sdio_ecran.elf' `
  -v -rst
```

Flash a raw binary at the Flash base address:

```powershell
& 'C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe' `
  -c port=SWD `
  -w 'build\F429_sdcard_sdio_ecran.bin' 0x08000000 `
  -v -rst
```

## Open COM39

```powershell
.\scripts\open_com.ps1 -Port COM39 -Baud 115200
```

Python alternative:

```powershell
python -m serial.tools.miniterm COM39 115200
```

> Close STM32CubeIDE serial consoles and any other terminal that already owns COM39 before opening it.

## SD card presence protocol

The firmware must emit one line per state change:

```text
SD:PRESENT
SD:REMOVED
SD:MOUNT_OK
SD:MOUNT_FAIL
SD:PART1_OK
SD:PART2_OK
```

Expected behavior:

1. Boot with no card: `SD:REMOVED`.
2. Insert a card: `SD:PRESENT`, then `SD:MOUNT_OK`, `SD:PART1_OK`, `SD:PART2_OK`.
3. Remove the card: `SD:REMOVED`.
4. Insert again: the same sequence repeats.

## Automated smoke test

```powershell
python .\tests\test_com39.py --port COM39 --baud 115200 --duration 20
```

The test reads lines from COM39 and checks the expected patterns. Run it while inserting and removing the card.

## Hardware notes

- The card-detect input is currently declared on `PA10` as `SD_CARD_DET`.
- `PA10` must be configured as an input with pull-up (active-low card-detect switch).
- `PA10` conflicts with `USART1_RX`. The serial log is therefore assumed to be **USART1 TX only on PA9** (or another UART after rewiring).
- If COM39 is not the ST-Link VCP, confirm the actual UART and pins before changing the firmware.

## Safety

- Confirm the ST-Link serial number and board before programming.
- Do not flash the `.bin` at the wrong address; use `0x08000000`.
- Do not open COM39 with two programs at the same time.
