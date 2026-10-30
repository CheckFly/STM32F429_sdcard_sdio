---
name: stm32-middleware
description: Integrate LVGL for the LTDC screen and FatFs for the SDIO card, including two partitions (4 GB + remaining space), card-detect handling on PA10, and COM39 status messages. Use when adding the display stack, the filesystem, or SD card presence detection.
license: MIT
compatibility: opencode
metadata:
  project: F429_sdcard_sdio_ecran
  display: LVGL + LTDC RGB565
  storage: SDIO 4-bit + FatFs
---

# STM32 middleware: LVGL, FatFs, and SD card

## What I do

- Plan and integrate LVGL on the LTDC display.
- Plan and integrate FatFs on the SDIO card.
- Support two partitions: partition 1 = 4 GB, partition 2 = remaining space.
- Implement SD card presence detection and COM39 messages.

## When to use me

Use me for display rendering, file access, SD card mounting, partitioning, and insert/remove tests.

## Current hardware facts

| Item | Value |
|------|-------|
| Screen | parked (display removed from the active build) |
| SD interface | SDIO **4-bit**: PC8..PC11 (D0..D3), PC12 (CK), PD2 (CMD) |
| SD clock | `ClockDiv = 4` → 7.5 MHz with a 45 MHz SDIOCLK |
| Card detect | `PA10`, label `SD_CARD_DET`, input pull-down |
| LCD conflict | `PC10` is `LTDC_R2`; it must never be configured as `SDIO_D2` |
| Active mode | UART-only diagnostics on COM39 |
| COM port | `COM39`, 115200 8N1 |
| Board | `32F429IDISCOVERY` |

> **PC10 note:** `PC10` is `LTDC_R2` on the on-board LCD. The display stack is parked, so PC10 is used as `SDIO_D2` in 4-bit mode. Re-validate before restoring LTDC.
>
> **4-bit bring-up:** initialize in 1-bit, enable `SDIO_HARDWARE_FLOW_CONTROL_ENABLE`, then call `HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B)` and fall back to 1-bit on error. 7.5 MHz (`ClockDiv = 4`) is the highest reliable divider; 9 MHz and above underrun the TX FIFO.
>
> **HAL pitfall:** stock `SD_FindSCR()` (wide-bus switch) reads the data FIFO without bounding the index and waits on `SDMMC_DATATIMEOUT` = `0xFFFFFFFF` ms. Bound the read to 2 words and the wait to ~250 ms, otherwise the switch can hang.
>
> **Important:** `PA10` is configured as `GPIO_Input` with pull-up, but the firmware reads it high with the card inserted **and** removed. The card-detect switch is not reaching `PA10` (or the module has no switch, or the active level is different). `PA10` is also `USART1_RX`; the serial log is TX-only on `PA9`.
>
> Before implementing FatFs mount-on-insert, verify the card-detect wiring with a multimeter:
> - card inserted: `PA10` should be low (active-low switch to GND),
> - card removed: `PA10` should be high (internal pull-up).
>
> If no card-detect switch is available, poll SDIO initialization instead and treat `HAL_SD_Init`/`HAL_SD_GetCardState` success as "card present".

## Storage layout (looper project)

| Partition | Size | MBR type | Use | FatFs |
|-----------|------|----------|-----|-------|
| 1 | 4 GB | `0xDA` non-FS | internal loops / overdubs / settings, raw block access | never mounted |
| 2 | remaining | `0x0C` FAT32 (0x07 > 32 GB) | project save/load | `0:` |

- `App/Src/sd_diskio.c` reads the MBR, picks the highest FAT-type entry and
  applies its start LBA. Access outside the partition returns `RES_PARERR`.
- `App/Src/sd_fs.c` mounts it; `SD_FS_SelfTest()` validates create/write/read.
- Keep `SD_CREATE_PARTITIONS_ON_INIT = 0` after the table is installed.
- MBR writes must be read back and retried: the first write after the 4-bit
  switch can be silently dropped.

## SD card presence state machine

```text
BOOT
 ├─ read PA10
 │   ├─ card present (low) → PRINT SD:PRESENT → mount → PRINT SD:MOUNT_OK
 │   └─ card absent  (high) → PRINT SD:REMOVED
 └─ loop every 200 ms
     ├─ stable present → mount if not mounted
     ├─ stable absent  → unmount → PRINT SD:REMOVED
     └─ state changed  → debounce 200 ms
```

Expected serial output:

```text
SD:REMOVED
SD:PRESENT
SD:MOUNT_OK
SD:PART1_OK
SD:PART2_OK
SD:REMOVED
```

## FATFS integration

1. Copy the FatFs middleware from the STM32Cube F4 package:

```powershell
$src = 'C:\Users\tmangear\STM32Cube\Repository\STM32Cube_FW_F4_V1.27.1\Middlewares\Third_Party\FatFs'
$dst = 'Middlewares\Third_Party\FatFs'
New-Item -ItemType Directory -Force -Path $dst | Out-Null
Copy-Item -Recurse "$src\src" "$dst\src"
Copy-Item "$src\doc" "$dst\doc"
```

2. Add `Middlewares/Third_Party/FatFs/src` to the include paths and source list.
3. Create `ffconf.h` from `ffconf_template.h` and enable:

```c
#define FF_USE_LFN      2
#define FF_MAX_LFN      255
#define FF_USE_MKFS     0
#define FF_FS_EXFAT     1   /* only if the card is larger than 32 GB */
#define FF_VOLUMES      2   /* two logical drives: 0: and 1: */
```

4. Adapt the SD diskio layer from the STM32Cube template:

```text
Middlewares/Third_Party/FatFs/src/drivers/sd_diskio.c
Middlewares/Third_Party/FatFs/src/drivers/sd_diskio.h
```

The diskio must:

- use the global `hsd` handle,
- map `pdrv == 0` to partition 1,
- map `pdrv == 1` to partition 2,
- read the MBR partition table once at init,
- apply the partition start LBA before `HAL_SD_ReadBlocks` / `HAL_SD_WriteBlocks`.

5. Mount both volumes:

```c
FATFS fs0, fs1;
f_mount(&fs0, "0:", 1);
f_mount(&fs1, "1:", 1);
```

6. Report results on COM39:

```c
printf("SD:PART1_OK\r\n");
printf("SD:PART2_OK\r\n");
```

## Two-partition layout

Recommended layout for a card of at least 8 GB:

| Partition | Size | Filesystem | Label | FatFs drive |
|-----------|------|------------|-------|-------------|
| 1 | 4 GB (4096 MB) | FAT32 | `SDCARD` | `0:` |
| 2 | remaining space | FAT32 (≤32 GB) or exFAT (>32 GB) | `DATA` | `1:` |

Partitioning is destructive. Use `scripts/partition_sdcard.ps1` only after verifying the disk number with `diskpart` → `list disk`. The script prints the diskpart commands by default and only executes them with `-Execute`.

## LVGL integration

1. Add LVGL as a submodule or vendored copy:

```powershell
git submodule add https://github.com/lvgl/lvgl.git Middlewares/Third_Party/LVGL
```

2. Copy `lv_conf_template.h` to `lv_conf.h` and set:

```c
#define LV_COLOR_DEPTH 16
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "main.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (HAL_GetTick())
```

3. Implement the display driver:

- `flush_cb` copies the rendered area into the LTDC frame buffer.
- The frame buffer lives in SDRAM (for example `0xC0000000`) or in internal RAM for a small area.
- Call `lv_disp_flush_ready()` after the copy.
- Configure `LV_COLOR_DEPTH 16` for RGB565.
- Use `HAL_LTDC_SetWindowPosition` and `HAL_LTDC_SetWindowSize` if the panel resolution is smaller than the LTDC timing.

4. Call `lv_init()` and `lv_timer_handler()` from the main loop.
5. Feed `lv_tick_inc()` from a 1 ms timer or use `HAL_GetTick()`.

> The current LTDC initialization has zero-sized layers and a null frame buffer. LVGL cannot render until the panel resolution, timings, and frame-buffer address are confirmed.

## Suggested module layout

```text
App/
├─ Inc/
│  ├─ log_uart.h
│  ├─ sd_monitor.h
│  └─ lvgl_port.h
└─ Src/
   ├─ log_uart.c
   ├─ sd_monitor.c
   └─ lvgl_port.c
Middlewares/
└─ Third_Party/
   ├─ FatFs/
   └─ LVGL/
```

## Test sequence

1. Build and flash.
2. Open COM39 at 115200.
3. Boot without a card → expect `SD:REMOVED`.
4. Insert the card → expect `SD:PRESENT`, `SD:MOUNT_OK`, `SD:PART1_OK`, `SD:PART2_OK`.
5. Remove the card → expect `SD:REMOVED`.
6. Insert again → expect the same sequence.
7. Run `tests/test_com39.py` and record the result in `docs/TESTS.md`.

## Safety

- Never run `diskpart clean` on the wrong disk.
- Verify the SD card capacity before choosing FAT32 vs exFAT.
- Do not enable `FF_FS_EXFAT` unless the FatFs version supports it and the card needs it.
- Keep the LVGL frame buffer out of the stack.
