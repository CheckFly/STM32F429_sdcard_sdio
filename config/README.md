# Configuration

`project.json` is the single source of truth for project metadata:

- author and version,
- board and MCU,
- COM port (COM39) and baud rate,
- display stack (LVGL / LTDC),
- storage stack (SDIO / FatFs / two partitions),
- build toolchain.

When a test is validated:

1. Update `version` in `config/project.json` (`0.01` → `0.02`).
2. Update `VERSION`.
3. Add an entry to `docs/HISTORY.md`.
4. Create a Git tag such as `v0.02`.
