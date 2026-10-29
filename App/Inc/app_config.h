#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Set to 1 to let the firmware create the MBR partitions after the SD card is
 * initialized. This is destructive: the card contents are erased. */
#define SD_CREATE_PARTITIONS_ON_INIT 0

/* Set to 1 to run the SD read/write throughput test after initialization.
 * The test writes 1 MB of data on the second partition area. */
#define SD_RUN_SPEED_TEST_ON_INIT    0

/* Set to 1 to sweep the SDIO clock dividers and keep the fastest reliable one. */
#define SD_RUN_CLOCK_SWEEP_ON_INIT   0

/* Number of 512-byte blocks used by the speed test (2048 blocks = 1 MB). */
#define SD_SPEED_TEST_BLOCKS         2048U

/* Set to 1 to switch the SD card to 4-bit wide bus after the 1-bit init.
 * Requires D1/D2/D3 wired to PC9/PC10/PC11. */
#define SD_USE_4BIT                  1

/* SDIO clock divider: SDIOCLK / (div + 2).
 * With SDIOCLK = 45 MHz (PLL48CLK from the 180 MHz PLL):
 *   0  -> 22.5 MHz  (TX FIFO underrun on write)
 *   2  -> 11.25 MHz (TX FIFO underrun on write)
 *   3  -> 9 MHz     (TX FIFO underrun on write)
 *   4  -> 7.5 MHz   (selected, 4-bit + DMA, verify OK)
 *   8  -> 4.5 MHz   (1-bit fallback was validated here)
 * Lower dividers underrun the TX FIFO during writes. */
#define SD_CLOCK_DIV                 4U

#endif /* APP_CONFIG_H */
