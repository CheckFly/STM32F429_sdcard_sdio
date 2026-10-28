# Test plan and results

## Test levels

| Level | What | Tool |
|-------|------|------|
| Build | Compile all sources and link | `scripts/build.ps1` / `make` |
| Flash | Program the board through ST-Link | `scripts/flash.ps1` |
| Serial | Read COM39 at 115200 | `scripts/open_com.ps1` |
| SD presence | Insert/remove detection | `tests/test_com39.py` |
| Filesystem | Mount `0:` and `1:` | firmware + COM39 logs |
| Display | LVGL test screen | visual check |

## Baseline results

| Date | Version | Test | Result |
|------|---------|------|--------|
| 2026-10-28 | 0.01 | STM32CubeIDE build | PASS — 14692 B text, 20 B data, 1868 B bss |
| 2026-10-28 | 0.01 | Portable `make` build | PASS — same size as CubeIDE |
| 2026-10-28 | 0.01 | Flash via STM32CubeProgrammer | PASS — ST-Link SN 0668FF555071494867204819 |
| 2026-10-28 | 0.01 | SD presence on COM39 | NOT RUN — firmware feature not implemented |
| 2026-10-28 | 0.01 | FatFs mount | NOT RUN — middleware not integrated |
| 2026-10-28 | 0.01 | LVGL display | NOT RUN — middleware not integrated |
| 2026-10-28 | 0.02 | USART1 TX / COM39 boot log | PASS — `SD:REMOVED` received at 115200 8N1 |
| 2026-10-28 | 0.02 | SD card absent detection | PASS — `PA10` reads high, `SD:REMOVED` emitted |
| 2026-10-28 | 0.02 | SD card present detection | FAIL — card present according to operator, but `PA10` reads high (`SD:REMOVED`) |
| 2026-10-28 | 0.02 | SD card removal detection | FAIL — no state change when the card was removed |
| 2026-10-28 | 0.02 | SD insert/remove cycle | BLOCKED — card-detect signal not reaching `PA10` |

### Card-detect finding

`PA10` is configured as input with pull-up. The firmware reads it high both with the card inserted and with the card removed. This means either:

1. the card-detect switch is not wired to `PA10`,
2. the module has no card-detect switch,
3. the signal is active-high and always high, or
4. the wire/ground is missing.

Do not rely on `SD_CARD_DET` until the physical wiring is confirmed.

## SD card presence test procedure

1. Build and flash version `0.0x`.
2. Open COM39:

   ```powershell
   .\scripts\open_com.ps1 -Port COM39 -Baud 115200
   ```

3. Boot without a card. Expect `SD:REMOVED`.
4. Insert the card. Expect:

   ```text
   SD:PRESENT
   SD:MOUNT_OK
   SD:PART1_OK
   SD:PART2_OK
   ```

5. Remove the card. Expect `SD:REMOVED`.
6. Insert again. Expect the same sequence.
7. Run the automated check:

   ```powershell
   python .\tests\test_com39.py --port COM39 --baud 115200 --duration 20 `
     --expect SD:PRESENT --expect SD:REMOVED
   ```

## Test result template

```markdown
| 2026-10-28 | 0.02 | SD insert/remove | PASS — SD:PRESENT, SD:MOUNT_OK, SD:PART1_OK, SD:PART2_OK, SD:REMOVED |
```

## Acceptance criteria

- No `SD:` message is duplicated without a state change.
- A card inserted before boot is detected after boot.
- Removing the card while mounted does not hang the firmware.
- Re-inserting the card recovers without a reset.
- Both partitions mount when present.
- The display remains responsive while the card is polled.
