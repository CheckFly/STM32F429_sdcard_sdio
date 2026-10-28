---
name: stm32-build
description: Compile the F429_sdcard_sdio_ecran STM32 project with STM32CubeIDE or the portable Makefile, and run the GitHub Actions build. Use for local builds, CI setup, toolchain discovery, and build error diagnosis.
license: MIT
compatibility: opencode
metadata:
  project: F429_sdcard_sdio_ecran
  ide: STM32CubeIDE 1.11.2
  toolchain: GNU Tools for STM32 14.3.rel1
---

# STM32 build workflow

## What I do

- Discover the STM32CubeIDE toolchain and GNU make on Windows.
- Build with `make` (portable, CI-friendly) or with STM32CubeIDE headless mode.
- Produce `.elf`, `.bin`, `.hex`, `.map`, and size output.
- Diagnose missing toolchain, include, linker, and path errors.

## When to use me

Use me before flashing, after changing sources or the `.ioc`, and when a CI build fails.

## Fast path on this machine

```powershell
# From the project root
.\scripts\build.ps1
```

The script adds the STM32CubeIDE toolchain to `PATH` and runs:

```powershell
make -j4
```

The binary is written to:

```text
build/F429_sdcard_sdio_ecran.elf
build/F429_sdcard_sdio_ecran.bin
build/F429_sdcard_sdio_ecran.hex
```

## Manual toolchain discovery (Windows)

```powershell
$gcc = Get-ChildItem 'C:\ST\STM32CubeIDE_*\STM32CubeIDE\plugins' -Directory |
    Where-Object Name -like 'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32*' |
    ForEach-Object {
        Get-ChildItem $_.FullName -Filter 'arm-none-eabi-gcc.exe' -Recurse -ErrorAction SilentlyContinue
    } |
    Select-Object -First 1
$gcc.FullName
```

Known path on this machine:

```text
C:\ST\STM32CubeIDE_1.11.2\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin
```

Also add a `make` directory:

```text
C:\ST\STM32CubeIDE_1.11.2\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.200.202604021615\tools\bin
```

## Portable build

```powershell
$env:PATH = "<gcc-bin>;<make-bin>;" + $env:PATH
make -j4
```

Clean:

```powershell
make clean
```

The root `Makefile` uses relative paths only, so it also works on Linux CI.

## STM32CubeIDE headless build

```powershell
& 'C:\ST\STM32CubeIDE_1.11.2\STM32CubeIDE\stm32cubeide.exe' `
  -nosplash `
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild `
  -data 'C:\Users\tmangear\STM32CubeIDE\workspace_1.11.2' `
  -build 'F429_sdcard_sdio_ecran/Debug'
```

Notes:

- `-data` must point to the workspace containing the project.
- `-build` accepts `<project>/<configuration>` (`Debug` or `Release`).
- Import first if the project is not in the workspace:

```powershell
& 'C:\ST\STM32CubeIDE_1.11.2\STM32CubeIDE\stm32cubeide.exe' `
  -nosplash `
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild `
  -data 'C:\Users\tmangear\STM32CubeIDE\workspace_1.11.2' `
  -import 'C:\Users\tmangear\STM32CubeIDE\workspace_1.11.2\F429_sdcard_sdio_ecran'
```

## Build configuration

- MCU: `STM32F429ZITx`
- FPU: `fpv4-sp-d16`, hard float
- Defines: `USE_HAL_DRIVER`, `STM32F429xx`, `DEBUG`
- Linker script: `STM32F429ZITX_FLASH.ld`
- Includes: `Core/Inc`, HAL `Inc`, CMSIS device, CMSIS core

## CI

The GitHub Actions workflow is `.github/workflows/build.yml`.

It installs `gcc-arm-none-eabi` and `make`, runs `make -j4`, and uploads the ELF/BIN/HEX as artifacts:

```bash
make -j"$(nproc)"
```

## Troubleshooting

| Symptom | Check |
|---------|-------|
| `arm-none-eabi-gcc: not found` | Add the CubeIDE GNU Tools `tools\bin` directory to `PATH`. |
| `make: not found` | Add the CubeIDE `make` plugin `tools\bin` directory to `PATH`. |
| Linker cannot find `STM32F429ZITX_FLASH.ld` | Run `make` from the project root, not from `Debug/`. |
| CubeIDE build is stale after adding files | Refresh the project in CubeIDE so it regenerates its makefiles, or use the portable Makefile. |
| `LOAD segment with RWX permissions` | Warning only; fixed later by refining the linker script. |
