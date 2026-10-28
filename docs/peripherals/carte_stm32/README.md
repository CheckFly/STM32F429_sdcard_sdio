# Carte STM32 — 32F429IDISCOVERY

## Hardware

| Item | Value |
|------|-------|
| Board | STM32F429I-DISCOVERY (32F429IDISCOVERY) |
| MCU | STM32F429ZIT6 |
| Package | LQFP144 |
| Core | Cortex-M4F |
| Flash | 2 MB |
| SRAM | 192 KB + 64 KB CCM |
| External RAM | 8 MB SDRAM (board dependent) |
| ST-Link | ST-LINK/V2-A, SWD |
| ST-Link serial | `0668FF555071494867204819` |
| Virtual COM | `COM39` (to confirm: USART1 TX PA9 or external adapter) |

## Clocks

The current `.ioc` runs from HSI (16 MHz). The board also provides an HSE source; the discovery examples use 8 MHz HSE and 180 MHz SYSCLK. Confirm the actual HSE value before changing the PLL.

## Memory map

| Region | Start | Size |
|--------|-------|------|
| Flash | `0x08000000` | 2048 KB |
| SRAM | `0x20000000` | 192 KB |
| CCM RAM | `0x10000000` | 64 KB |
| SDRAM (if wired) | `0xC0000000` (typical) | 8 MB |

> The SDRAM base address depends on the FMC bank and the board wiring. Confirm before using it as an LVGL frame buffer.

## Debug / flash

```powershell
.\scripts\flash.ps1
```

ST-Link command:

```powershell
STM32_Programmer_CLI.exe -c port=SWD -w build\F429_sdcard_sdio_ecran.elf -v -rst
```

## Tests

- Build: `.\scripts\build.ps1`
- Flash: `.\scripts\flash.ps1`
- Serial: `.\scripts\open_com.ps1 -Port COM39`

## Known issues

- The board's on-board LCD is an SPI ILI9341; this project uses an external RGB panel through LTDC.
- `PA10` is assigned to `SD_CARD_DET`; verify against the physical wiring.
- The current clock is conservative (16 MHz HSI).

## References

- STM32F429ZIT6 datasheet
- STM32F429I-DISCOVERY user manual (UM1670)
- STM32F4 HAL reference manual
