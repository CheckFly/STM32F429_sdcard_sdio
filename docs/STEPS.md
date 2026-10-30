# Project steps

## Phase 0 — Baseline

- [x] Analyze the existing CubeIDE project.
- [x] Build the baseline firmware.
- [x] Flash the baseline firmware.
- [x] Record the baseline size in `HISTORY.md`.
- [x] Add portable build and CI.
- [x] Add project, build, flash, docs, middleware, and Git skills.

## Phase 1 — Serial logging and SD detection

- [x] Confirm COM39 is the ST-Link Virtual COM Port.
- [x] Configure USART1 TX on `PA9` at 115200 8N1.
- [x] Implement `LOG_Printf` (polling transmit).
- [x] Change `PA10` from output to input with pull-up.
- [x] Implement 200 ms debounce for `SD_CARD_DET`.
- [x] Emit `SD:REMOVED` when `PA10` is high.
- [ ] Confirm card-detect wiring and emit `SD:PRESENT` when the card is inserted.
- [ ] Test insert, remove, and re-insert.
- [ ] Run `tests/test_com39.py`.
- [x] Bump version to `0.02` and tag `v0.02` (pending Git commit).

## Phase 2 — FatFs and two partitions

> **Status:** the display is parked; the SD card is validated over UART in **4-bit SDIO**
> (PC8..PC11 = D0..D3, PC12 CK, PD2 CMD) at 7.5 MHz (`ClockDiv = 4`) with DMA.
> Throughput: 2509 KB/s write / 2876 KB/s read, verify OK.

- [x] Validate SDIO 1-bit initialization over UART.
- [x] Record the card capacity/type from `SD:CARD` (7580 MB, 512-byte blocks).
- [x] Validate 4-bit wide-bus operation and DMA throughput.
- [x] Create the MBR with 4 GB + remaining-space partitions.
- [x] Copy FatFs from STM32Cube F4 V1.27.1.
- [x] Add `ffconf.h` (single volume, static LFN buffer).
- [x] Implement `sd_diskio.c` using the global `hsd`.
- [x] Parse the MBR and map `0:` to the FAT partition (partition 2).
- [x] Mount the volume and emit `SD:FS_MOUNT_OK` + `SD:FS_USAGE`.
- [ ] Run the file self-test (`SD_FS_SELFTEST_ON_INIT`).
- [ ] Test a 4 GB FAT32 partition and the remaining-space partition.
- [ ] Tag `v0.10` once the file self-test passes.

## Phase 3 — LVGL and LTDC

- [ ] Confirm the panel resolution, timings, and frame-buffer address.
- [ ] Configure LTDC layers with the correct window size.
- [ ] Allocate the frame buffer (SDRAM preferred).
- [ ] Add LVGL and `lv_conf.h` (`LV_COLOR_DEPTH = 16`).
- [ ] Implement the LVGL flush callback.
- [ ] Display a test screen and a status screen.
- [ ] Bump version to `0.04` and tag `v0.04`.

## Phase 4 — Integration and release

- [ ] Show SD status on the LCD.
- [ ] Add a FATFS file browser or a simple read/write demo.
- [ ] Add CI artifact upload and release notes.
- [ ] Finalize `docs/peripherals/*`.
- [ ] Tag `v1.00` when the complete feature set is validated.

## Current blockers

| Blocker | Impact | Action |
|---------|--------|--------|
| No FatFs diskio | No file access | Add an MBR-aware diskio layer |
| SDIO above 7.5 MHz | TX FIFO underrun on writes | Keep `ClockDiv = 4`; needs a faster feed path |
| Display parked | No visible output | Restore LTDC/LVGL, then re-check PC10 (SDIO_D2) |
