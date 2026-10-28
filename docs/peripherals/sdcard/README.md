# SD card — SDIO + FatFs

## Hardware

| Item | Value |
|------|-------|
| Interface | SDIO 4-bit |
| Data pins | PC8 (D0), PC9 (D1), PC10 (D2), PC11 (D3) |
| Clock | PC12 (CK) |
| Command | PD2 (CMD) |
| Card detect | PA10 (`SD_CARD_DET`), **must be input with pull-up** |
| Pull-ups | Verify the module has 10 kΩ pull-ups on CMD and D0..D3 |

## Current configuration

- `MX_SDIO_SD_Init` sets `SDIO_BUS_WIDE_4B` and `HAL_SD_ConfigWideBusOperation`.
- The SD clock divider is `0` in the `.ioc`; verify the resulting clock is within the card's capability.
- FatFs sources have been copied into `Middlewares/Third_Party/FatFs`, but the diskio layer is not integrated yet.
- `PA10` is now configured as `GPIO_Input` with pull-up.
- **Test result:** `PA10` reads high with the card inserted and with the card removed. The card-detect switch is not reaching the MCU, or the module has no switch, or the active level/wiring is different.
- SDIO initialization is currently disabled in `main.c` to keep the presence monitor alive.

## Partitions

| Partition | Size | Filesystem | Label | FatFs drive |
|-----------|------|------------|-------|-------------|
| 1 | 4 GB | FAT32 | `SDCARD` | `0:` |
| 2 | remaining space | FAT32 (≤32 GB) or exFAT (>32 GB) | `DATA` | `1:` |

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
