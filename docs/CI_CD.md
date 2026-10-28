# Build and CI

## Local build

```powershell
.\scripts\build.ps1
```

The script finds `arm-none-eabi-gcc.exe` and `make.exe` under `C:\ST` and runs `make -j4`.

Manual build:

```powershell
$env:PATH = "<gcc-bin>;<make-bin>;" + $env:PATH
make -j4
```

Outputs:

```text
build/F429_sdcard_sdio_ecran.elf
build/F429_sdcard_sdio_ecran.bin
build/F429_sdcard_sdio_ecran.hex
build/F429_sdcard_sdio_ecran.map
```

## STM32CubeIDE build

See the `stm32-build` skill. The portable Makefile is the recommended CI path because it uses only relative paths.

## GitHub Actions

Workflow: `.github/workflows/build.yml`

It runs on push and pull request:

1. Checkout.
2. Install `gcc-arm-none-eabi`, `make`, and `binutils-arm-none-eabi`.
3. `make -j$(nproc)`.
4. Upload `build/F429_sdcard_sdio_ecran.elf`, `.bin`, `.hex`, and `.map` as artifacts.

The workflow does **not** flash hardware. Flashing is done locally or on a self-hosted runner with the board attached.

## Future hardware CI

A self-hosted runner can flash and run the serial smoke test:

```yaml
runs-on: [self-hosted, windows, stm32]
steps:
  - uses: actions/checkout@v4
  - run: .\scripts\build_and_flash.ps1
  - run: python .\tests\test_com39.py --port COM39 --duration 20
```

The runner must have:

- STM32CubeProgrammer installed,
- the board connected through ST-Link,
- COM39 free,
- a physical operator or fixture to insert/remove the SD card.

## Versioning

- Build artifacts are ignored by Git.
- Releases are tagged `v0.0x`.
- The GitHub release should attach the ELF/BIN/HEX produced by the workflow.
