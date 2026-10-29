# SD card — SDIO + FatFs

## Hardware

| Item | Value |
|------|-------|
| Interface | SDIO 4-bit (DMA) |
| Data pins | PC8 (D0), PC9 (D1), PC10 (D2), PC11 (D3) |
| Clock | PC12 (CK) |
| Command | PD2 (CMD) |
| Card detect | PA10 (`SD_CARD_DET`), input pull-down, active-high |
| Pull-ups | Adafruit 4682 provides them; internal pull-ups are also enabled |

## Current configuration

- The firmware uses **4-bit SDIO**:
  - `PC8..PC11` = `SDIO_D0..D3`
  - `PC12` = `SDIO_CK`
  - `PD2` = `SDIO_CMD`
- The card is initialized in 1-bit mode first, then switched with
  `HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B)`. If the switch fails,
  the driver falls back to 1-bit and logs `SD:BUS 1-bit`.
- `SDIO_HARDWARE_FLOW_CONTROL_ENABLE` is set.
- `PA10` is configured as `GPIO_Input` with pull-down (active-high card detect). The bench test detects insertion/removal through the UART.
- SDIO clock: `hsd.Init.ClockDiv = 4` → **7.5 MHz** with a 45 MHz SDIOCLK.
- DMA is enabled:
  - TX: DMA2 Stream6, Channel4
  - RX: DMA2 Stream3, Channel4
  - `DMA_PFCTRL`, FIFO full, peripheral burst INC4, 32-byte-aligned buffers, VERY_HIGH priority
- **Validated:** `SD:INIT_OK`, `SD:WIDE_OK`, `SD:BUS 4-bit`, `SD:CARD`, MBR partitions, and 512 KB write/read with DMA:
  - CPU 180 MHz, SDIO 4-bit DMA at 7.5 MHz
  - write ≈ 2509 KB/s
  - read ≈ 2876 KB/s
  - verify OK
  - partition 2 start (LBA 8,390,656) also verified OK in 1-bit mode
- Divider sweep at 4-bit: div=8 OK, div=4 OK (best), div=3 and div=2 fail with
  TX FIFO underrun (`err=4096`).
- FatFs sources are copied into `Middlewares/Third_Party/FatFs`, but the diskio layer is not integrated.
- `SD_ENABLE_SDIO` is set to `1` in `App/Src/sd_monitor.c`; destructive tests and clock sweep are disabled.

## Known pitfall — `SD_FindSCR()`

The stock HAL `SD_FindSCR()` (used by the wide-bus switch) reads the data FIFO
without bounding the index and waits with `SDMMC_DATATIMEOUT` = `0xFFFFFFFF` ms,
i.e. ~49 days. On this project the 4-bit switch hung at 7.5 MHz. The local HAL
copy is patched to read at most 2 words and to time out after 250 ms.

## Partitions

> Supported cards: **8 GB and larger**. Smaller cards are reported as
> `SD:PARTITIONS unsupported (8GB+ only)` and will be handled later.
> The second partition size is computed from the **real capacity** returned by
> `HAL_SD_GetCardInfo()`, not hardcoded.

| Partition | Size | Filesystem | Label | FatFs drive |
|-----------|------|------------|-------|-------------|
| 1 | 4 GB (4096 MB) | FAT32 | `SDCARD` | `0:` |
| 2 | total capacity − 4096 MB − 1 MB | FAT32 (≤32 GB) or exFAT (>32 GB) | `DATA` | `1:` |

Example plans (real capacity is typically lower than the marketed size):

| Card | Real capacity | Partition 1 | Partition 2 |
|------|---------------|-------------|-------------|
| 8 GB | ≈ 7,400 MB | 4,096 MB | ≈ 3,303 MB |
| 16 GB | ≈ 14,900 MB | 4,096 MB | ≈ 10,803 MB |
| 32 GB | ≈ 29,800 MB | 4,096 MB | ≈ 25,703 MB |
| 4 GB | ≈ 3,700 MB | — | unsupported for now |

### Partitioning

Use `scripts/partition_sdcard.ps1`:

```powershell
# Dry run: print the diskpart script
.\scripts\partition_sdcard.ps1 -DiskNumber 2

# Apply after checking the disk number
.\scripts\partition_sdcard.ps1 -DiskNumber 2 -Execute
```

> `diskpart clean` erases the entire card. Verify the disk number with `diskpart` → `list disk` first.

## FatFs plan

1. Copy `ff.c`, `ff.h`, `ffconf.h`, `diskio.c`, `diskio.h`, `ff_gen_drv.c`, `ff_gen_drv.h`, `integer.h` into `Middlewares/Third_Party/FatFs/src`.
2. Add `sd_diskio.c`/`.h` adapted from the STM32Cube template.
3. Read the MBR at LBA 0 and store the start LBA and sector count of both partitions.
4. Map `pdrv = 0` to partition 1 and `pdrv = 1` to partition 2.
5. Apply the partition start LBA in `disk_read`/`disk_write`.
6. Mount:

   ```c
   FATFS fs0, fs1;
   f_mount(&fs0, "0:", 1);
   f_mount(&fs1, "1:", 1);
   ```

7. Report:

   ```text
   SD:MOUNT_OK
   SD:PART1_OK
   SD:PART2_OK
   ```

## Card presence

State machine:

```text
read PA10 (active low)
  low  → present
  high → absent
debounce 200 ms
on change:
  present → print SD:PRESENT, mount, print results
  absent  → unmount, print SD:REMOVED
```

## Test procedure

1. Flash the firmware.
2. Open COM39.
3. Boot without a card → `SD:REMOVED`.
4. Insert a card → `SD:PRESENT`, `SD:MOUNT_OK`, `SD:PART1_OK`, `SD:PART2_OK`.
5. Remove the card → `SD:REMOVED`.
6. Re-insert → same sequence.
7. Run `python tests/test_com39.py --port COM39`.

## Known issues

- `PA10` is currently an output.
- The second partition requires MBR-aware diskio.
- exFAT requires `FF_FS_EXFAT` and a FatFs version that supports it.
- SDIO pull-ups and signal integrity must be checked on the actual board.
