---
name: stm32-docs
description: Generate and maintain STM32 project documentation in Markdown and Doxygen, including the shared docs tree, peripheral notes, history, project steps, and API documentation. Use when documenting a peripheral, a test, a release, or the build/CI process.
license: MIT
compatibility: opencode
metadata:
  project: F429_sdcard_sdio_ecran
  format: Markdown + Doxygen
---

# STM32 documentation workflow

## What I do

- Maintain `docs/` as the shared documentation root.
- Keep one folder per used peripheral under `docs/peripherals/`.
- Keep `docs/HISTORY.md` and `docs/STEPS.md` up to date.
- Generate Doxygen HTML/LaTeX from the C sources.
- Produce test and release notes.

## When to use me

Use me after a feature, a test, a pin change, a new peripheral, or a release.

## Documentation layout

```text
docs/
├─ README.md                 # index and navigation
├─ HISTORY.md                # chronological change log
├─ STEPS.md                  # project milestones and next actions
├─ PINOUT.md                 # pin table and conflicts
├─ CI_CD.md                  # build and CI explanation
├─ TESTS.md                  # test plan and results
└─ peripherals/
   ├─ carte_stm32/README.md
   ├─ sdcard/README.md
   ├─ memoire_ajoutee/README.md
   ├─ ecran/README.md
   └─ port_com/README.md
```

## Markdown rules

- One H1 per file.
- Use tables for pin maps and test results.
- Use fenced code blocks with a language tag (`powershell`, `bash`, `c`, `text`).
- Date entries as `YYYY-MM-DD`.
- Put the **what**, **why**, and **how to reproduce** in each entry.
- Never delete history; append and correct with a new entry.

## History entry template

```markdown
## 2026-10-28 — v0.01 — Initial build and flash

- Built with GNU Tools for STM32 14.3.rel1.
- Flashed `F429_sdcard_sdio_ecran.elf` through ST-Link/SWD.
- Added the portable Makefile, GitHub Actions workflow, and project skills.
- Next: LVGL, FatFs, SD presence detection.
```

## Project step template

```markdown
### Step 3 — SD card presence detection

- [x] Declare `SD_CARD_DET` on PA10.
- [ ] Configure PA10 as input with pull-up.
- [ ] Emit `SD:PRESENT` / `SD:REMOVED` on COM39.
- [ ] Debounce the signal (200 ms).
- [ ] Validate insertion/removal with `tests/test_com39.py`.
```

## Doxygen

Create a `Doxyfile` at the project root if missing:

```powershell
doxygen -g Doxyfile
```

Recommended settings:

```text
PROJECT_NAME           = "F429_sdcard_sdio_ecran"
PROJECT_NUMBER         = "0.01"
OUTPUT_DIRECTORY       = docs/doxygen
INPUT                  = Core/Src Core/Inc
RECURSIVE              = YES
GENERATE_HTML          = YES
GENERATE_LATEX         = NO
EXTRACT_ALL            = YES
```

Generate:

```powershell
doxygen Doxyfile
```

The HTML entry point is `docs/doxygen/html/index.html`.

## Peripheral note template

```markdown
# SD card (SDIO + FatFs)

## Hardware
- Interface: SDIO 4-bit
- Pins: PC8..PC12, PD2
- Card detect: PA10 (`SD_CARD_DET`, active low)

## Software
- Driver: `stm32f4xx_hal_sd.c`
- Filesystem: FatFs
- Partitions: `0:` 4 GB FAT32, `1:` remaining space

## Initialization
1. Configure SDIO pins.
2. `HAL_SD_Init`.
3. `HAL_SD_ConfigWideBusOperation`.
4. `f_mount`.

## Tests
- Insert card → `SD:PRESENT`.
- Remove card → `SD:REMOVED`.

## Limitations
- `PA10` must be input, not output.
- Second partition requires an MBR-aware diskio layer.
```

## Release note template

```markdown
# v0.02

## Added
- SD card presence detection on COM39.
- FatFs mount test for both partitions.

## Fixed
- `PA10` configured as input with pull-up.

## Tested
- Cold boot without card.
- Insert/remove/insert cycle.
```

## Safety

- Do not document an unverified pin or voltage as fact.
- Mark assumptions with `> Assumption:`.
- Keep generated Doxygen output out of Git (`docs/doxygen/`).
