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

- [ ] Copy FatFs from STM32Cube F4 V1.27.1.
- [ ] Add `ffconf.h` with two volumes.
- [ ] Implement `sd_diskio.c` using the global `hsd`.
- [ ] Parse the MBR and map `0:` and `1:` to the two partitions.
- [ ] Mount both volumes and emit `SD:MOUNT_OK`, `SD:PART1_OK`, `SD:PART2_OK`.
- [ ] Test a 4 GB FAT32 partition and the remaining-space partition.
- [ ] Bump version to `0.03` and tag `v0.03`.

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
| COM39 UART unknown | Serial log cannot be transmitted | Confirm wiring / ST-Link VCP UART |
| `PA10` output mode | Card detect may drive the signal instead of reading it | Reconfigure as input with pull-up |
| LTDC zero-size layers | No visible output | Set panel timings and frame buffer |
| No FatFs diskio | No file access | Add MBR-aware diskio |
| No LVGL sources | No GUI | Add LVGL middleware |
