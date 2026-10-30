# F429_sdcard_sdio_ecran — Documentation

Project: STM32F429ZI board with SD card (SDIO + FatFs) and an RGB LCD driven by LTDC/LVGL.

## Current status

| Item | Status |
|------|--------|
| Build with STM32CubeIDE GCC toolchain | ✅ verified |
| Build with portable `Makefile` | ✅ verified |
| Flash through ST-Link/SWD | ✅ verified (board: 32F429IDISCOVERY) |
| Display | ⏸ parked; UART-only mode for now |
| FatFs on SDIO | ✅ mounted on partition 2 as `0:` (FAT32, 3482 MB) |
| Two partitions (4 GB internal + remaining FAT32) | ✅ 0xDA internal + 0x0C FAT32 LBA |
| COM39 serial log (USART1 TX) | ✅ verified |
| SD presence on COM39 | ✅ `SD:PRESENT` / `SD:REMOVED` |
| SDIO 4-bit | ✅ `SD:WIDE_OK`, D0..D3 on PC8..PC11 |
| SDIO 1-bit fallback | ✅ `SD:BUS 1-bit` if the wide-bus switch fails |
| MBR partitions (4 GB + rest) | ✅ created via `SD_CreatePartitions()` |
| Write/read throughput test | ✅ 2509/2876 KB/s at 180 MHz CPU + 7.5 MHz 4-bit DMA |
| Card validation | ✅ h2testw full card, no errors |
| Partition 2 write | ✅ 532 KB/s, verify OK at LBA 8,390,656 |
| File self-test (create/write/read/delete) | ⏳ flag `SD_FS_SELFTEST_ON_INIT`, to run on the bench |
| CPU clock | ✅ 180 MHz (HSI PLL, 5 WS, APB1 45 MHz, APB2 90 MHz) |
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

1. **PC10 is also `LTDC_R2` on the on-board LCD:** the display stack is parked, so PC10 is free for `SDIO_D2`. Re-check signal integrity before restoring LTDC.
2. SDIO is in **4-bit** mode at 7.5 MHz (`SD_CLOCK_DIV = 4`). Dividers below 4 underrun the TX FIFO during writes.
3. `PA10` is configured as `GPIO_Input` with pull-down (active-high card detect). It is also `USART1_RX`; the COM39 log is USART1 TX-only on `PA9`.
4. `SD_ENABLE_SDIO` is set to `1` in `App/Src/sd_monitor.c`. The generated `MX_SDIO_SD_Init()` call must be removed after a CubeMX regeneration; the App manages SDIO in `App/Src/sd_card.c`.
5. The two-partition FatFs access requires an MBR-aware diskio layer mapping logical drives `0:` and `1:` to partition offsets.
6. The CPU runs at 180 MHz from HSI/PLL (5 WS, APB1 45 MHz, APB2 90 MHz). `SystemClock_Config()` is customized; CubeMX regeneration will revert it unless the RCC/PLL settings are updated in the `.ioc`.
