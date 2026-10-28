# Project history

## 2026-10-28 — v0.01 — Initial build, flash, and tooling

### Done

- Analyzed the STM32CubeIDE project.
- Identified `STM32F429ZITx` / `32F429IDISCOVERY`, 2 MB Flash, 192 KB RAM, 64 KB CCM.
- Verified the existing CubeIDE project structure and HAL configuration.
- Built the project with GNU Tools for STM32 14.3.rel1:
  - `text = 14692 B`, `data = 20 B`, `bss = 1868 B`.
- Flashed `F429_sdcard_sdio_ecran.elf` through ST-Link/SWD with STM32CubeProgrammer 2.12.0.
- Added a portable root `Makefile` and verified it produces the same firmware.
- Added PowerShell scripts for build, flash, serial monitoring, and SD card partitioning.
- Added GitHub Actions build workflow.
- Added OpenCode skills for project, build, flash, documentation, middleware, and Git.
- Created the shared documentation tree with one folder per used peripheral.
- Added `config/project.json`, `VERSION`, `AUTHORS.md`, and `.gitignore`.

### Findings

- `PA10` is declared as `SD_CARD_DET` but configured as an output. It must be an input with pull-up.
- `PA10` is also `USART1_RX`; the COM39 logging path must be confirmed.
- The LTDC layers are initialized with zero size and a null frame buffer.
- No FatFs or LVGL middleware is present yet.
- The current clock is 16 MHz HSI; the project does not yet use the board's HSE/PLL.

### Next

- Configure the UART used by COM39.
- Configure `PA10` as input with pull-up and implement debounced card detection.
- Add FatFs with an MBR-aware two-partition diskio layer.
- Add LVGL and the LTDC display port.
- Validate insertion/removal with `tests/test_com39.py`.

## 2026-10-28 — v0.02 — USART1 log and SD presence monitor

### Done

- Enabled `HAL_UART_MODULE_ENABLED` and added the USART1 HAL driver to the project.
- Added `MX_USART1_UART_Init()`: USART1 TX only on `PA9`, 115200 8N1.
- Added `App/Src/log_uart.c` and `App/Inc/log_uart.h`.
- Added `App/Src/sd_monitor.c` and `App/Inc/sd_monitor.h`.
- Changed `PA10` from `GPIO_Output` to `GPIO_Input` with pull-up in `main.c` and in the `.ioc`.
- Moved UART and SD-monitor initialization before SDIO init so a missing card cannot block the boot.
- Made SDIO initialization non-fatal and skipped it when the card is absent.
- Verified COM39 reception:
  - `SD:REMOVED` is received at 115200 8N1.
  - USART1 registers show `USART1EN`, `TE`, and the correct baud rate.
- Verified `PA10` card-detect input is read by the firmware.

### Findings

- COM39 is the ST-Link Virtual COM Port (VID 0483, PID 374B).
- The ST community reports that the VCP uses USART1 (PA9/PA10) and requires solder bridges SB11/SB15 on some boards.
- With the card reportedly present, the boot log showed rapid `SD:PRESENT` / `SD:REMOVED` transitions. This was caused by serial-port/DTR resets during the test, not by the card.
- With SDIO initialization disabled, the card-present and card-removed tests both report `SD:REMOVED`: `PA10` reads high in both cases.
- SDIO initialization was temporarily disabled to isolate the presence detection.
- The card-detect signal is not reaching `PA10`, or the module has no card-detect switch, or the active level is different.

### Next

- Confirm the SD card detect wiring and active level.
- If the module has a CD/SW pin, wire it to `PA10` (active low to GND when inserted).
- Re-enable SDIO initialization only after the card-detect behavior is stable.
- Implement FatFs and the two-partition diskio layer.
- Add LVGL and the LTDC display port.
