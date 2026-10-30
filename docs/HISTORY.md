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
- Initialized the Git repository on `main`, committed the snapshot, and tagged `v0.02`.

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

## 2026-10-28 — v0.03 — LVGL and hardware conflict found

- Integrated LVGL v8.3.11 with a 240x320 RGB565 frame buffer in SDRAM.
- The on-board LCD is now driven by the official BSP (`BSP_LCD_Init`) with the correct ILI9341 240x320 timing.
- The status screen shows the version and the card presence with LVGL labels.
- **Critical hardware finding:** the on-board LCD uses **PC10 as LTDC_R2**, while SDIO 4-bit uses PC10 as SDIO_D2. The two functions are physically incompatible.
- The SDIO driver was switched to **1-bit mode** (PC8 D0, PC12 CK, PD2 CMD). `HAL_SD_MspDeInit` no longer touches PC10, which previously corrupted the LCD after a failed SD init.
- `SD_ENABLE_SDIO` is temporarily set to `0` in `App/Src/sd_monitor.c` to keep the display stable until the module's D1/D2/D3 lines are disconnected and the 1-bit wiring is verified.
- `SD:INIT_FAIL` is expected while SDIO is disabled or while D2 is still connected to PC10.

## 2026-10-28 — v0.04 — UART-only SD validation

- Removed the display/LVGL code from the active build. The display is parked in `App/display_parked/`.
- The project now uses **UART only** for SD diagnostics:
  - `=== F429_sdcard_sdio_ecran v0.03 ===`
  - `SD:PRESENT` / `SD:REMOVED`
  - `SD:INIT_OK` / `SD:INIT_FAIL err=...`
  - `SD:CARD type=... blocks=... block_size=... capacity=...MB`
- Switched the SD driver to **1-bit SDIO** (PC8 D0, PC12 CK, PD2 CMD) and lowered the clock with `ClockDiv = 8` (4.8 MHz with a 48 MHz SDIOCLK).
- Removed `HAL_SD_ConfigWideBusOperation` because the card's SCR register reported no wide-bus support (`SD:WIDE_FAIL err=0x04000000`).
- **Validated:** `SD:INIT_OK` and `SD:CARD` are received on COM39.
- Added `SD:CAPACITY`, `SD:PART1`, and `SD:PART2` output. The second partition is computed from the card's real capacity (`total − 4096 MB − 1 MB`).
- Supported cards: **8 GB and larger**; smaller cards print `SD:PARTITIONS unsupported (8GB+ only)` for now.
- The App-managed SD initialization lives in `App/Src/sd_card.c`; after a CubeMX regeneration, remove the generated `MX_SDIO_SD_Init()` call from `main.c`.

## 2026-10-28 — v0.05 — Partitions and throughput tests

- Added `SD_CreatePartitions()` in `App/Src/sd_card.c`: writes a verified MBR with
  partition 1 at LBA 2048 (4 GB) and partition 2 at LBA 8,390,656 (remaining space).
- Added `SD_TestWriteSpeed()`, `SD_TestReadSpeed()`, and `SD_TestReadWrite()` in
  `App/Src/sd_test.c`.
- Added `SD_WaitReady()` and read retries to handle the card's busy state after writes.
- Validated at `ClockDiv = 64` (0.73 MHz, 1-bit SDIO):
  - partition creation: PASS
  - write 1 MB at LBA 2048: 83 KB/s
  - read 1 MB at LBA 2048: 84 KB/s
  - verify: OK
- Writing at partition 2 start (≈4 GB) failed with `CMD_RSP_TIMEOUT` / `ILLEGAL_CMD`.
  This strongly suggests a counterfeit/short card or a card that cannot use the
  upper 4 GB. Test with h2testw/f3 on a PC.
- The destructive flags are set to 0 after validation.

## 2026-10-29 — v0.06 — SDIO clock sweep

- h2testw validated the full card: 7561/7562 MB, no errors, write 10.1 MB/s, read 41.2 MB/s.
- Added `SD_TestClockSweep()` to test SDIO dividers 64, 32, 16, 8 with a 64 KB
  write/read at LBA 2048.
- Results:
  - div=64 (0.73 MHz): 84/83 KB/s
  - div=32 (1.41 MHz): 158/156 KB/s
  - div=16 (2.67 MHz): 281/277 KB/s — selected
  - div=8 (4.8 MHz): TX underrun / CMD timeout
- `SD_CLOCK_DIV` is now **16** and the destructive tests/sweep are disabled.
- Faster options: DMA transfers, 4-bit SDIO (if D1/D2/D3 are wired), or a higher
  CPU clock.

## 2026-10-29 — v0.07 — SDIO DMA

- Added SDIO DMA:
  - TX: DMA2 Stream6, Channel4
  - RX: DMA2 Stream3, Channel4
  - `DMA_PFCTRL`, FIFO full, peripheral burst INC4, 32-byte-aligned buffers
  - DMA2 Stream3/Stream6 IRQs and SDIO IRQ handled.
- `SD_TestWriteSpeed()` / `SD_TestReadSpeed()` / `SD_TestReadWrite()` and
  `SD_CreatePartitions()` now use `HAL_SD_*Blocks_DMA()`.
- Sweep with DMA:
  - div=64: 85/84 KB/s
  - div=32: 160/158 KB/s
  - div=16: 288/281 KB/s
  - div=8: 477/460 KB/s — selected
- `SD_CLOCK_DIV = 8` (4.8 MHz), 1-bit SDIO with DMA.
- The destructive tests and the sweep are disabled after validation.

## 2026-10-29 — v0.08 — CPU at 180 MHz

- Switched `SystemClock_Config` to 180 MHz:
  - HSI 16 MHz, PLLM 16, PLLN 360, PLLP 2
  - PLLQ 8 → 45 MHz PLL48CLK for SDIO
  - APB1 45 MHz, APB2 90 MHz, flash latency 5 WS, voltage scale 1
- SDIO remains 1-bit with DMA at `SD_CLOCK_DIV = 8`.
- 1 MB test at partition 2 start (LBA 8,390,656):
  - write 532 KB/s
  - read 524 KB/s
  - verify OK
- Partition 2 is therefore writable on the validated card.
- The UART remains at 115200 8N1 and the board boots correctly.
- Destructive SD tests are disabled after validation.

## 2026-10-29 — v0.09 — SDIO 4-bit

- SDIO now runs in **4-bit** mode:
  - `PC8..PC11` = D0..D3, `PC12` = CK, `PD2` = CMD
  - internal pull-ups on D0..D3 and CMD, no pull on CK
  - `HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE`
  - init in 1-bit, then `HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B)`
  - fallback to 1-bit if the wide-bus switch fails
- `SD_RUN_SPEED_TEST_ON_INIT` runs a 1 MB test; SWEEP tests 512 KB per divider.
- 4-bit results (180 MHz CPU, SDIOCLK 45 MHz):
  - div=8 (4.5 MHz): 1777/1868 KB/s
  - div=4 (7.5 MHz): 2509/2876 KB/s — selected
  - div=3 (9 MHz) and div=2 (11.25 MHz): TX FIFO underrun
- `SD_CLOCK_DIV = 4`, i.e. **7.5 MHz**, 4-bit SDIO with DMA.
- HAL fix in `SD_FindSCR()`: bounded FIFO read (2 words) and 250 ms timeout
  instead of `SDMMC_DATATIMEOUT` (0xFFFFFFFF ms). Without it the 4-bit switch
  hung at 7.5 MHz.
- Added `SD:INIT_START` / `SD:INIT_CARD_OK` / `SD:WIDE_START` traces and a
  HardFault logger (`SD:FAULT cfsr/hfsr/pc/lr`).
- The sweep and destructive tests are disabled in the final build.

## 2026-10-30 — v0.10 — FatFs on the data partition

Storage layout decided for the looper application:

- **Partition 1** (4 GB): internal raw area for loops, overdubs and settings.
  MBR type `0xDA` (non-filesystem) so it never appears as a drive on a PC.
  The firmware accesses it directly by LBA; FatFs never mounts it.
- **Partition 2** (remaining space, 3483 MB): user-visible FAT32 volume for
  project save/load, mounted as `0:`.

Changes:

- `App/Src/sd_diskio.c`: FatFs diskio over `HAL_SD_*Blocks_DMA()`.
  - reads the MBR to locate the FAT partition (highest FAT entry wins),
  - refuses access outside the partition,
  - bounce buffer for unaligned FatFs pointers,
  - 3 retries per transfer, error logging `SD:FS_READ_FAIL` / `SD:FS_WRITE_FAIL`.
- `App/Src/sd_fs.c`: `SD_FS_Mount()` / `SD_FS_Unmount()` / `SD_FS_LogUsage()` /
  `SD_FS_LogRoot()` / `SD_FS_SelfTest()`, FAT32 format via `f_mkfs()` when asked.
- Build: `ff.c`, `ff_gen_drv.c`, `diskio.c` and `option/unicode.c` added;
  `_VOLUMES = 1`, `_USE_LFN = 1` (static LFN buffer, no heap).
- `SD_CreatePartitions()` now writes the MBR with write + read-back verify and
  retries: the first write after the 4-bit switch can be silently dropped.
- The format path is blocked unless the partition table was validated, so a
  failure can no longer format the internal partition by mistake.
- Validated on COM39:
  - `SD:PART1 LBA=2048 sectors=8388608 type=0xDA (internal)`
  - `SD:PART2 LBA=8390656 sectors=7133184 type=0x0C`
  - `SD:FS_FORMAT_OK`
  - `SD:FS_MOUNT_OK drive=0:/ lba=8390656 sectors=7133184`
  - `SD:FS_USAGE total=3482MB free=3482MB cluster=32768B`
- `SD_FS_SELFTEST_ON_INIT` (file create/write/read/delete) is available but off
  by default; it still has to be run on the bench.
