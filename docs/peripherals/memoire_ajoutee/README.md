# Added memory — SDRAM / external RAM

## Internal memory

| Region | Start | Size | Use |
|--------|-------|------|-----|
| Flash | `0x08000000` | 2048 KB | Code and constants |
| SRAM | `0x20000000` | 192 KB | Data, stack, heap |
| CCM RAM | `0x10000000` | 64 KB | Fast data (no DMA) |

## External SDRAM

The 32F429IDISCOVERY board provides an 8 MB SDRAM device connected to the FMC. The exact part and bank depend on the board revision.

| Item | Value |
|------|-------|
| Size | 8 MB (64 Mbit) |
| Interface | FMC SDRAM |
| Typical base | `0xC0000000` |
| Use | LVGL frame buffer, large buffers |

## Why it matters for LVGL

A 480 × 272 RGB565 frame buffer uses:

```text
480 × 272 × 2 = 261,120 bytes ≈ 255 KB
```

This does not fit in the 192 KB SRAM with the rest of the application. Put the LVGL frame buffer in SDRAM.

A double buffer doubles the requirement:

```text
2 × 255 KB ≈ 510 KB
```

The 8 MB SDRAM has enough room.

## TODO

- [ ] Confirm the SDRAM part and FMC bank on the actual board.
- [ ] Add the FMC/SDRAM initialization from the BSP or CubeMX.
- [ ] Add the SDRAM region to the linker script if variables are placed there.
- [ ] Verify read/write with a memory test before using it for LVGL.
- [ ] Document the frame-buffer address in `docs/peripherals/ecran/README.md`.

## Safety

- Do not place the stack or heap in SDRAM before verifying the FMC timing.
- Do not assume the base address; check the board schematic.
- Keep DMA buffers in SRAM or CCM, not in SDRAM, unless the FMC/DMA path is validated.
