# F429_sdcard_sdio_ecran — Documentation

Project: STM32F429ZI board with SD card (SDIO + FatFs) and an RGB LCD driven by LTDC/LVGL.

## Current status

| Item | Status |
|------|--------|
| Build with STM32CubeIDE GCC toolchain | ✅ verified |
| Build with portable `Makefile` | ✅ verified |
| Flash through ST-Link/SWD | ✅ verified (board: 32F429IDISCOVERY) |
| LVGL display stack | ⏳ planned |
| FatFs on SDIO | ⏳ planned |
| Two partitions (4 GB + remaining space) | ⏳ planned |
| SD presence on COM39 | ⏳ planned |
| GitHub Actions build | ✅ added |

## Quick start

```powershell
# Build
.\scripts\build.ps1

# Build and flash
.\scripts\build_and_flash.ps1

# Open COM39
.\scripts\open_com.ps1 -Port COM39 -Baud 115200

# Watch only SD status lines
.\scripts\monitor_sd.ps1 -Port COM39
```

## Navigation

| Document | Content |
|----------|---------|
| [HISTORY.md](HISTORY.md) | Chronological change log |
| [STEPS.md](STEPS.md) | Project milestones and next actions |
| [PINOUT.md](PINOUT.md) | Pins, alternate functions, conflicts |
| [CI_CD.md](CI_CD.md) | Build and CI pipeline |
| [TESTS.md](TESTS.md) | Test plan and results |
| [peripherals/carte_stm32/README.md](peripherals/carte_stm32/README.md) | Board and MCU |
| [peripherals/sdcard/README.md](peripherals/sdcard/README.md) | SDIO, FatFs, partitions |
| [peripherals/memoire_ajoutee/README.md](peripherals/memoire_ajoutee/README.md) | SDRAM / external memory |
| [peripherals/ecran/README.md](peripherals/ecran/README.md) | LTDC, LVGL, RGB565 |
| [peripherals/port_com/README.md](peripherals/port_com/README.md) | COM39 and SD status protocol |

## Project facts

| Item | Value |
|------|-------|
| Board | `32F429IDISCOVERY` |
| MCU | `STM32F429ZITx`, LQFP144 |
| Flash / RAM | 2 MB / 192 KB + 64 KB CCM |
| External RAM | 8 MB SDRAM (board dependent) |
| COM port | `COM39`, 115200 8N1 |
| Display | LVGL over LTDC, RGB565 |
| Storage | SDIO 4-bit + FatFs |
| Partitions | `0:` 4 GB FAT32, `1:` remaining space |
| Toolchain | GNU Tools for STM32 14.3.rel1 |

## Known issues / decisions to confirm

1. `PA10` is now configured as `GPIO_Input` with pull-up, but it reads high with the card inserted and removed. The card-detect switch is not reaching the MCU, or its active level/wiring is different.
2. `PA10` is also `USART1_RX`; the COM39 log is USART1 TX-only on `PA9`. Do not enable USART1 RX while `PA10` is used for card detection.
3. The LTDC layers are initialized with zero size and a null frame buffer; LVGL cannot render until the panel resolution and frame-buffer address are set.
4. The two-partition FatFs access requires an MBR-aware diskio layer mapping logical drives `0:` and `1:` to partition offsets.
5. The current clock is 16 MHz HSI; LVGL and SD performance will improve with a proper PLL configuration (for example 180 MHz).
