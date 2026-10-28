# Peripherals

One folder per peripheral actually used by the project.

| Folder | Peripheral | Status |
|--------|------------|--------|
| [carte_stm32](carte_stm32/README.md) | Board and MCU | documented |
| [sdcard](sdcard/README.md) | SDIO, FatFs, partitions | hardware known, middleware planned |
| [memoire_ajoutee](memoire_ajoutee/README.md) | SDRAM / external RAM | documented, FMC TODO |
| [ecran](ecran/README.md) | LTDC + LVGL | LTDC skeleton, LVGL planned |
| [port_com](port_com/README.md) | COM39 serial | settings known, UART mapping to confirm |

## Adding a peripheral

1. Create a folder with a lowercase name.
2. Add a `README.md` using the template in the `stm32-docs` skill.
3. Add the wiring, pins, initialization, tests, and limitations.
4. Link it from this index and from `docs/README.md`.
