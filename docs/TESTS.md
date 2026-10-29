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

### PC10 / LCD conflict

The official BSP and UM1670 show that **PC10 is `LCD_TFT R2`** on the 32F429IDISCOVERY board. SDIO 4-bit uses PC10 as `SDIO_D2`. This is a hardware conflict:

- SDIO 4-bit cannot be used with the on-board LCD.
- When SDIO initialization reconsidered PC10, the LCD image was corrupted and the firmware could hang.
- The current firmware uses **1-bit SDIO** (PC8 D0, PC12 CK, PD2 CMD) and leaves PC10 to the LTDC.
- `HAL_SD_MspDeInit` no longer de-initializes PC10.
- `SD_ENABLE_SDIO` is set to `0` in `App/Src/sd_monitor.c` until the module's D1/D2/D3 lines are disconnected and the 1-bit wiring is verified.

### UART-only validation

| Date | Version | Test | Result |
|------|---------|------|--------|
| 2026-10-28 | 0.04 | UART banner | PASS |
| 2026-10-28 | 0.04 | SDIO 1-bit init | PASS — `SD:INIT_OK` |
| 2026-10-28 | 0.04 | SD card info | PASS — `SD:CARD type/blocks/block_size/capacity` received |
| 2026-10-28 | 0.04 | Capacity-based partition plan | PASS — `SD:CAPACITY`, `SD:PART1`, `SD:PART2` printed |
| 2026-10-28 | 0.04 | 8 GB+ policy | PASS — smaller cards report `SD:PARTITIONS unsupported (8GB+ only)` |
| 2026-10-28 | 0.04 | MBR creation | PASS — `SD:PART1 LBA=2048 sectors=8388608`, `SD:PART2 LBA=8390656 sectors=7133184` |
| 2026-10-28 | 0.04 | Write/read 1 MB at LBA 2048 | PASS — write 83 KB/s, read 84 KB/s, verify OK, at 0.73 MHz 1-bit |
| 2026-10-28 | 0.04 | Write at partition 2 start (LBA 8,390,656 ≈ 4 GB) | FAIL — `CMD_RSP_TIMEOUT` / `ILLEGAL_CMD`; suspect counterfeit/short card |
| 2026-10-28 | 0.04 | SDIO 4-bit wide bus | FAIL — `SD:WIDE_FAIL err=0x04000000` (no wide-bus support) |
| 2026-10-29 | 0.06 | h2testw full card | PASS — 7561/7562 MB, no errors, write 10.1 MB/s, read 41.2 MB/s |
| 2026-10-29 | 0.06 | Clock sweep div=64 (0.73 MHz) | PASS — 84/83 KB/s |
| 2026-10-29 | 0.06 | Clock sweep div=32 (1.41 MHz) | PASS — 158/156 KB/s |
| 2026-10-29 | 0.06 | Clock sweep div=16 (2.67 MHz) | PASS — 281/277 KB/s, chosen |
| 2026-10-29 | 0.06 | Clock sweep div=8 (4.8 MHz) | FAIL — TX underrun then CMD timeout (polling) |
| 2026-10-29 | 0.07 | SDIO DMA div=8 (4.8 MHz) | PASS — 477/460 KB/s |
| 2026-10-29 | 0.08 | CPU 180 MHz + DMA div=8 | PASS — 532/524 KB/s, verify OK |
| 2026-10-29 | 0.08 | Partition 2 write (LBA 8,390,656) | PASS — 532 KB/s, verify OK |
| 2026-10-29 | 0.09 | SDIO 4-bit wide bus | PASS — `SD:WIDE_OK`, 1777/1875 KB/s |
| 2026-10-29 | 0.09 | 4-bit sweep div=4 (7.5 MHz) | PASS — 2509/2876 KB/s, verify OK, selected |
| 2026-10-29 | 0.09 | 4-bit sweep div=3 (9 MHz) | FAIL — TX underrun (err=4096) |
| 2026-10-29 | 0.09 | 4-bit sweep div=2 (11.25 MHz) | FAIL — TX underrun (err=4096) |
| 2026-10-28 | 0.04 | Display | PARKED — removed from the active build |

The current firmware is UART-only:

- `SD_ENABLE_SDIO = 1`
- **4-bit SDIO**: `PC8..PC11` = D0..D3, `PC12` = CK, `PD2` = CMD
- CPU 180 MHz; PLL48/SDIOCLK = 45 MHz; `SD_CLOCK_DIV = 4` → 7.5 MHz
- SDIO DMA (DMA2 Stream6 TX, Stream3 RX, PFCTRL/FIFO/burst, VERY_HIGH priority)
- hardware flow control enabled, 1-bit init then `HAL_SD_ConfigWideBusOperation(4B)`
- card info and partition plan printed on COM39
- destructive tests disabled after validation

### SDIO clock sweep

A 64 KB write/read test was run at each divider (SDIOCLK = 48 MHz):

| Divider | SDIO clock | Write | Read | Result |
|---------|------------|-------|------|--------|
| 64 | 0.73 MHz | 85 KB/s | 84 KB/s | OK |
| 32 | 1.41 MHz | 160 KB/s | 158 KB/s | OK |
| 16 | 2.67 MHz | 288 KB/s | 281 KB/s | OK |
| 8 | 4.8 MHz | 477 KB/s | 460 KB/s | **selected** |

With the DMA configuration from CubeMX (`DMA_PFCTRL`, FIFO full, peripheral
burst INC4, aligned buffers), the 4.8 MHz step now passes. The current
`SD_CLOCK_DIV` is **8**, 1-bit SDIO with DMA.

The CPU is now at **180 MHz**:

- HSI 16 MHz → PLLM 16 → 1 MHz
- PLLN 360 → 360 MHz VCO
- PLLP 2 → 180 MHz SYSCLK
- PLLQ 8 → 45 MHz PLL48CLK (SDIO)
- APB1 45 MHz, APB2 90 MHz, flash 5 WS

At 180 MHz, the 1 MB test at the start of **partition 2** (LBA 8,390,656)
passes:

```text
write = 532 KB/s
read  = 524 KB/s
verify = OK
```

This is about 6.4× faster than the original 0.73 MHz polling configuration, and
it confirms that partition 2 is writable on the validated card.

### 4-bit wide bus

The card runs in 4-bit mode after the 1-bit initialization
(`HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B)`), with hardware flow
control enabled and internal pull-ups on D0..D3 and CMD.

Sweep with 512 KB write/read at LBA 2048, SDIOCLK = 45 MHz:

| Divider | SDIO clock | Write | Read | Result |
|---------|------------|-------|------|--------|
| 8 | 4.5 MHz | 1777 KB/s | 1868 KB/s | OK |
| 4 | 7.5 MHz | 2509 KB/s | 2876 KB/s | **selected** |
| 3 | 9 MHz | — | — | TX underrun `err=4096` |
| 2 | 11.25 MHz | — | — | TX underrun `err=4096` |
| 0 | 22.5 MHz | — | — | TX underrun `err=4098` |

`SD_CLOCK_DIV = 4` (7.5 MHz) is the fastest divider that passes the write/read
verification. Beyond it the SDIO TX FIFO underruns during the first block write.

Compared with the 1-bit DMA result (532/524 KB/s), 4-bit gives roughly
**4.7× more write throughput** and **5.5× more read throughput**.

> HAL fix: `SD_FindSCR()` in `stm32f4xx_hal_sd.c` read the FIFO without an index
> bound and used `SDMMC_DATATIMEOUT` (`0xFFFFFFFF` ms ≈ 49 days) as its timeout.
> Besides the stack overflow, a missing ACMD51 response could hang the firmware
> during the wide-bus switch. The read is now bounded to 2 words and the wait to
> 250 ms.

### Card capacity validation

Writing at the start of partition 1 (LBA 2048) works. An earlier attempt to write
at the start of partition 2 (LBA 8,390,656 ≈ 4 GB) failed with `CMD_RSP_TIMEOUT` /
`ILLEGAL_CMD`, which suggested a counterfeit/short card.

**h2testw on the full card passed:**

```text
Only 7561 of 7562 MByte tested.
Test finished without errors.
Writing speed: 10.1 MByte/s
Reading speed: 41.2 MByte/s
```

The card is therefore genuine and full-capacity. The partition 2 failure was a
driver/card-state issue (the card was busy after the MBR write), not a fake card.
Retest partition 2 with the wait/retry logic and the selected clock divider.

The destructive flags are now disabled:

```c
#define SD_CREATE_PARTITIONS_ON_INIT 0
#define SD_RUN_SPEED_TEST_ON_INIT    0
```

Re-enable them only when a known-good card is inserted.

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
