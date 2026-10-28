---
name: stm32-project
description: Create and maintain STM32 project templates, version files, author/config metadata, and the shared documentation tree for the F429_sdcard_sdio_ecran project. Use when scaffolding a new STM32 project, bumping the 0.0x template version, or adding peripheral documentation folders.
license: MIT
compatibility: opencode
metadata:
  board: 32F429IDISCOVERY
  mcu: STM32F429ZITx
  project: F429_sdcard_sdio_ecran
---

# STM32 project and template workflow

## What I do

- Create a new project from `templates/project_template_0.01`.
- Bump the template version (`0.01` → `0.02`) after each validated test.
- Create or update `VERSION`, `config/project.json`, `AUTHORS.md`.
- Create the shared documentation tree under `docs/`.
- Keep the template and the live project consistent.

## When to use me

Use me when starting a new STM32 project, when a test milestone is validated, or when a new peripheral must be documented.

## Project facts

| Item | Value |
|------|-------|
| Board | `32F429IDISCOVERY` |
| MCU | `STM32F429ZITx` |
| Flash / RAM | 2 MB / 192 KB + 64 KB CCM |
| COM port | `COM39`, 115200 8N1 |
| Display | LVGL over LTDC, RGB565 |
| Storage | SDIO 4-bit + FatFs, partition 1 = 4 GB, partition 2 = remaining space |

## Versioning rule

- Template versions use the form `0.0x`.
- Increment the last digit after every validated test: `0.01`, `0.02`, `0.03`, …
- At `0.09`, the next version is `0.10`.
- Tag the Git commit with `v0.0x` (for example `v0.02`).
- Add a line to `docs/HISTORY.md` for each version.

PowerShell helper:

```powershell
$versionFile = "VERSION"
$current = (Get-Content $versionFile -Raw).Trim()
if ($current -match '^0\.(\d{2})$') {
    $next = '0.{0:D2}' -f ([int]$Matches[1] + 1)
} else {
    $next = '0.01'
}
Set-Content -Path $versionFile -Value $next -NoNewline
(Get-Content config/project.json -Raw) -replace '"version"\s*:\s*"[^"]+"', ('"version": "' + $next + '"') |
    Set-Content config/project.json
Write-Host "Template version: $next"
```

> On PowerShell 5 the format expression above is not valid; use:
> `$next = '0.' + (([int]$Matches[1] + 1).ToString('00'))`

## Create a new template from the current one

```powershell
$source = "templates/project_template_0.01"
$target = "templates/project_template_0.02"
if (Test-Path $target) { throw "Target already exists: $target" }
Copy-Item -Recurse $source $target
Set-Content "$target/VERSION.txt" "0.02" -NoNewline
```

## Documentation tree

Create one folder per peripheral actually used by the project:

```text
docs/
├─ README.md
├─ HISTORY.md
├─ STEPS.md
├─ PINOUT.md
├─ CI_CD.md
├─ TESTS.md
└─ peripherals/
   ├─ carte_stm32/
   │  └─ README.md
   ├─ sdcard/
   │  └─ README.md
   ├─ memoire_ajoutee/
   │  └─ README.md
   ├─ ecran/
   │  └─ README.md
   └─ port_com/
      └─ README.md
```

Each peripheral `README.md` must contain:

1. Hardware description and wiring.
2. Pins used and alternate functions.
3. Initialization sequence.
4. Test procedure.
5. Known limitations / TODO.

## Config metadata

`config/project.json` is the source of truth. It must contain at least:

- `name`, `version`, `author`, `created`
- `board`, `mcu`
- `com_port`, `com_baudrate`
- `display`, `storage`, `build`

Do not duplicate the version in several places without updating all of them.

## Safety

- Do not overwrite an existing template folder; create a new versioned folder.
- Keep generated build output (`Debug/`, `build/`) out of Git.
- Record every validated test in `docs/HISTORY.md` and `docs/STEPS.md`.
