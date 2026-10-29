#ifndef APP_SD_TEST_H
#define APP_SD_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
    uint32_t start_lba;
    uint32_t block_count;
    uint32_t write_ms;
    uint32_t read_ms;
    uint32_t write_kbs;
    uint32_t read_kbs;
    uint32_t verify_errors;
    int success;
} SD_SpeedTestResult;

typedef struct
{
    uint32_t div;
    uint32_t write_kbs;
    uint32_t read_kbs;
    uint32_t verify_errors;
    int success;
} SD_ClockResult;

/**
 * @brief Write block_count blocks at start_lba and measure the throughput.
 *
 * The data is written in 4 KB chunks filled with a deterministic pattern.
 * The card contents at the target location are overwritten.
 */
int SD_TestWriteSpeed(
    uint32_t start_lba,
    uint32_t block_count,
    SD_SpeedTestResult *result);

/**
 * @brief Read block_count blocks at start_lba and measure the throughput.
 */
int SD_TestReadSpeed(
    uint32_t start_lba,
    uint32_t block_count,
    SD_SpeedTestResult *result);

/**
 * @brief Write, then read back and verify block_count blocks.
 *
 * Reports both write and read throughput and counts verification errors.
 */
int SD_TestReadWrite(
    uint32_t start_lba,
    uint32_t block_count,
    SD_SpeedTestResult *result);

/**
 * @brief Try several SDIO clock dividers and keep the fastest reliable one.
 *
 * Tested dividers: 64 (0.73 MHz), 32 (1.41 MHz), 16 (2.67 MHz), 8 (4.8 MHz).
 * Each step writes/reads block_count blocks at start_lba. The card is left
 * initialized at the fastest divider that passed.
 */
int SD_TestClockSweep(
    uint32_t start_lba,
    uint32_t block_count,
    SD_ClockResult *best);

#ifdef __cplusplus
}
#endif

#endif /* APP_SD_TEST_H */
