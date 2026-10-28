# FatFs middleware

Source: STM32Cube F4 V1.27.1 / FatFs R0.12c.

## Status

- [x] FatFs source copied into `src/`.
- [x] `ffconf.h` created with two volumes and exFAT enabled.
- [ ] `sd_diskio.c` adapted to the project `hsd` handle.
- [ ] MBR parsing for two partitions.
- [ ] Mount `0:` and `1:`.
- [ ] Build integration (Makefile and STM32CubeIDE).

## Configuration

`ffconf.h` currently sets:

```c
#define _VOLUMES   2
#define _USE_LFN   3
#define _MAX_LFN   255
#define _FS_EXFAT  1
```

## Integration plan

1. Create `src/drivers/sd_diskio.c` and `sd_diskio.h`.
2. Implement `disk_initialize`, `disk_status`, `disk_read`, `disk_write`, `disk_ioctl`.
3. Read the MBR at LBA 0.
4. Store the start LBA and sector count of partition 1 and partition 2.
5. Map `pdrv = 0` to partition 1 and `pdrv = 1` to partition 2.
6. Add the FatFs sources and include paths to the build.
7. Mount both volumes and report the result on COM39.

## Do not

- Do not add FatFs to the build until `sd_diskio.c` is implemented; the generic `diskio.c` references RAM/USB disk drivers that are not present.
- Do not enable `_FS_EXFAT` unless the card is formatted exFAT and the Flash budget is acceptable.
