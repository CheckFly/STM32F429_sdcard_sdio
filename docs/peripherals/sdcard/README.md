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

| Partition | Size | MBR type | Filesystem | Purpose | FatFs drive |
|-----------|------|----------|------------|---------|-------------|
| 1 | 4 GB (4096 MB) | `0xDA` non-FS | none (raw) | internal looper area: loops, overdubs, settings | — |
| 2 | total capacity − 4096 MB − 1 MB | `0x0C` (FAT32 LBA) or `0x07` (>32 GB) | FAT32 / exFAT | user projects, save and load | `0:` |

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

## FatFs

Implemented in `App/Src/sd_diskio.c` and `App/Src/sd_fs.c`.

- `sd_diskio.c` implements the FatFs `disk_*` callbacks on top of
  `HAL_SD_ReadBlocks_DMA()` / `HAL_SD_WriteBlocks_DMA()`:
  - reads LBA 0 at init to locate the FAT partition (highest FAT-type entry),
  - applies the partition start LBA to every access and rejects out-of-range
    requests,
  - uses an aligned 512-byte bounce buffer when the caller pointer is not
    word-aligned,
  - retries each transfer 3 times and logs `SD:FS_READ_FAIL` / `SD:FS_WRITE_FAIL`.
- `sd_fs.c` exposes `SD_FS_Mount()`, `SD_FS_Unmount()`, `SD_FS_LogUsage()`,
  `SD_FS_LogRoot()` and `SD_FS_SelfTest()`. It can create the FAT32 volume with
  `f_mkfs()` when `SD_FORMAT_PART2_ON_INIT = 1`.
- FatFs build settings: `_VOLUMES = 1`, `_USE_LFN = 1` (static LFN buffer, no
  heap), `_MAX_SS = 512`, `_USE_MKFS = 1`, `_FS_EXFAT = 1`.
- `option/unicode.c` is compiled for `ff_wtoupper()`.

Reported on COM39:

```text
SD:PART1 LBA=2048 sectors=8388608 type=0xDA (internal)
SD:PART2 LBA=8390656 sectors=7133184 type=0x0C
SD:FS_FORMAT_OK
SD:FS_MOUNT_OK drive=0:/ lba=8390656 sectors=7133184
SD:FS_USAGE total=3482MB free=3482MB cluster=32768B
```

### Internal partition is invisible on a PC

Partition 1 uses MBR type `0xDA` ("non-filesystem data"), so Windows and Linux
do not assign it a drive letter. The firmware will access it directly by LBA;
FatFs only ever mounts partition 2. Note that "invisible" means "not mounted",
not "protected": a partitioning tool can still overwrite it.

### Self-test

Set `SD_FS_SELFTEST_ON_INIT = 1` to create `FSTEST.TXT`, write it, read it back,
compare and delete it. Expected log:

```text
SD:FS_TEST_START
SD:FS_TEST OK bytes=38
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

- `SD_CreatePartitions()` must run once to install the 0xDA + 0x0C table; after
  that keep `SD_CREATE_PARTITIONS_ON_INIT = 0`.
- The first write after the 4-bit switch can be dropped by the card; the MBR
  writer therefore verifies by read-back and retries.
- The file self-test has not been run on the bench yet.
- exFAT requires `FF_FS_EXFAT`; the MBR type switches to `0x07` above 32 GB.
- PC10 (`SDIO_D2`) is shared with `LTDC_R2`; re-check if the display returns.
