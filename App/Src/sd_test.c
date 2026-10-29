#include "sd_test.h"

#include <string.h>

#include "log_uart.h"
#include "sd_card.h"

#define SD_TEST_CHUNK_BLOCKS 64U
#define SD_TEST_BUFFER_WORDS 8192U

static uint32_t sd_test_buffer[SD_TEST_BUFFER_WORDS] __attribute__((aligned(32)));

static void SD_Test_FillBuffer(uint32_t pattern)
{
    uint32_t index;

    for (index = 0U; index < SD_TEST_BUFFER_WORDS; ++index)
    {
        sd_test_buffer[index] = pattern;
    }
}

static uint32_t SD_Test_Kbps(uint32_t blocks, uint32_t elapsed_ms)
{
    if (elapsed_ms == 0U)
    {
        elapsed_ms = 1U;
    }

    return (blocks * 512U * 1000U) / (elapsed_ms * 1024U);
}

int SD_TestWriteSpeed(
    uint32_t start_lba,
    uint32_t block_count,
    SD_SpeedTestResult *result)
{
    uint32_t offset;
    uint32_t chunk;
    uint32_t pattern;
    uint32_t start_tick;
    uint32_t elapsed;

    if (result == NULL || block_count == 0U)
    {
        return 0;
    }

    (void)memset(result, 0, sizeof(*result));
    result->start_lba = start_lba;
    result->block_count = block_count;

    LOG_Printf("SD:TEST_WRITE_START LBA=%lu (destructive)\r\n", (unsigned long)start_lba);

    (void)SD_WaitReady(2000U);
    HAL_Delay(50U);

    start_tick = HAL_GetTick();

    for (offset = 0U; offset < block_count; offset += chunk)
    {
        chunk = block_count - offset;
        if (chunk > SD_TEST_CHUNK_BLOCKS)
        {
            chunk = SD_TEST_CHUNK_BLOCKS;
        }

        pattern = 0x5AA5AA55U ^ offset;
        SD_Test_FillBuffer(pattern);

        {
            HAL_StatusTypeDef write_status = HAL_ERROR;
            uint32_t retry;

            for (retry = 0U; retry < 3U; ++retry)
            {
                write_status = HAL_SD_WriteBlocks_DMA(
                    &hsd,
                    (uint8_t *)sd_test_buffer,
                    start_lba + offset,
                    chunk);
                if (write_status == HAL_OK)
                {
                    if (SD_WaitTransfer(5000U))
                    {
                        break;
                    }
                    write_status = HAL_ERROR;
                }

                LOG_Printf("SD:TEST_WRITE_RETRY %lu err=%lu\r\n",
                           (unsigned long)retry, (unsigned long)hsd.ErrorCode);
                (void)SD_WaitReady(1000U);
                HAL_Delay(100U);
            }

            if (write_status != HAL_OK)
            {
                LOG_Printf("SD:TEST_WRITE_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
                return 0;
            }
        }
    }

    elapsed = HAL_GetTick() - start_tick;
    if (elapsed == 0U)
    {
        elapsed = 1U;
    }

    result->write_ms = elapsed;
    result->write_kbs = SD_Test_Kbps(block_count, elapsed);
    result->success = 1;

    LOG_Printf(
        "SD:TEST_WRITE blocks=%lu bytes=%lu time=%lu ms speed=%lu KB/s\r\n",
        (unsigned long)block_count,
        (unsigned long)(block_count * 512U),
        (unsigned long)elapsed,
        (unsigned long)result->write_kbs);

    return 1;
}

int SD_TestReadSpeed(
    uint32_t start_lba,
    uint32_t block_count,
    SD_SpeedTestResult *result)
{
    uint32_t offset;
    uint32_t chunk;
    uint32_t start_tick;
    uint32_t elapsed;

    if (result == NULL || block_count == 0U)
    {
        return 0;
    }

    LOG_Write("SD:TEST_READ_START\r\n");

    start_tick = HAL_GetTick();

    for (offset = 0U; offset < block_count; offset += chunk)
    {
        chunk = block_count - offset;
        if (chunk > SD_TEST_CHUNK_BLOCKS)
        {
            chunk = SD_TEST_CHUNK_BLOCKS;
        }

        if (HAL_SD_ReadBlocks_DMA(&hsd, (uint8_t *)sd_test_buffer, start_lba + offset, chunk) != HAL_OK)
        {
            LOG_Printf("SD:TEST_READ_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
            return 0;
        }

        if (!SD_WaitTransfer(5000U))
        {
            LOG_Printf("SD:TEST_READ_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
            return 0;
        }
    }

    elapsed = HAL_GetTick() - start_tick;
    if (elapsed == 0U)
    {
        elapsed = 1U;
    }

    result->read_ms = elapsed;
    result->read_kbs = SD_Test_Kbps(block_count, elapsed);

    LOG_Printf(
        "SD:TEST_READ blocks=%lu bytes=%lu time=%lu ms speed=%lu KB/s\r\n",
        (unsigned long)block_count,
        (unsigned long)(block_count * 512U),
        (unsigned long)elapsed,
        (unsigned long)result->read_kbs);

    return 1;
}

int SD_TestReadWrite(
    uint32_t start_lba,
    uint32_t block_count,
    SD_SpeedTestResult *result)
{
    uint32_t offset;
    uint32_t chunk;
    uint32_t pattern;
    uint32_t start_tick;
    uint32_t elapsed;
    uint32_t errors = 0U;
    uint32_t index;

    if (result == NULL || block_count == 0U)
    {
        return 0;
    }

    if (!SD_TestWriteSpeed(start_lba, block_count, result))
    {
        return 0;
    }

    (void)SD_WaitReady(2000U);
    HAL_Delay(50U);

    LOG_Write("SD:TEST_VERIFY_START\r\n");

    start_tick = HAL_GetTick();

    for (offset = 0U; offset < block_count; offset += chunk)
    {
        chunk = block_count - offset;
        if (chunk > SD_TEST_CHUNK_BLOCKS)
        {
            chunk = SD_TEST_CHUNK_BLOCKS;
        }

        if (HAL_SD_ReadBlocks_DMA(&hsd, (uint8_t *)sd_test_buffer, start_lba + offset, chunk) != HAL_OK)
        {
            LOG_Printf("SD:TEST_READ_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
            return 0;
        }

        if (!SD_WaitTransfer(5000U))
        {
            LOG_Printf("SD:TEST_READ_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
            return 0;
        }

        pattern = 0x5AA5AA55U ^ offset;
        for (index = 0U; index < SD_TEST_BUFFER_WORDS; ++index)
        {
            if (sd_test_buffer[index] != pattern)
            {
                ++errors;
            }
        }
    }

    elapsed = HAL_GetTick() - start_tick;
    if (elapsed == 0U)
    {
        elapsed = 1U;
    }

    result->read_ms = elapsed;
    result->read_kbs = SD_Test_Kbps(block_count, elapsed);
    result->verify_errors = errors;
    result->success = (errors == 0U) ? 1 : 0;

    LOG_Printf(
        "SD:TEST_READ blocks=%lu bytes=%lu time=%lu ms speed=%lu KB/s verify_errors=%lu\r\n",
        (unsigned long)block_count,
        (unsigned long)(block_count * 512U),
        (unsigned long)elapsed,
        (unsigned long)result->read_kbs,
        (unsigned long)errors);

    LOG_Printf(
        "SD:TEST_RESULT write=%lu KB/s read=%lu KB/s verify=%s\r\n",
        (unsigned long)result->write_kbs,
        (unsigned long)result->read_kbs,
        (result->success ? "OK" : "FAIL"));

    return result->success;
}

int SD_TestClockSweep(
    uint32_t start_lba,
    uint32_t block_count,
    SD_ClockResult *best)
{
    static const uint32_t dividers[] = {4U, 3U, 2U};
    SD_ClockResult best_result = {0U, 0U, 0U, 0U, 0};
    uint32_t best_div = 64U;
    uint32_t index;
    int found = 0;

    if (best != NULL)
    {
        *best = best_result;
    }

    for (index = 0U; index < (sizeof(dividers) / sizeof(dividers[0])); ++index)
    {
        uint32_t div = dividers[index];
        SD_SpeedTestResult result;

        if (!SD_Card_InitAtClock(div))
        {
            LOG_Printf("SD:SWEEP div=%lu init=FAIL\r\n", (unsigned long)div);
            continue;
        }

        if (!SD_TestReadWrite(start_lba, block_count, &result))
        {
            LOG_Printf("SD:SWEEP div=%lu test=FAIL\r\n", (unsigned long)div);
            continue;
        }

        LOG_Printf(
            "SD:SWEEP div=%lu write=%lu KB/s read=%lu KB/s verify=OK\r\n",
            (unsigned long)div,
            (unsigned long)result.write_kbs,
            (unsigned long)result.read_kbs);

        if (!found || result.write_kbs > best_result.write_kbs)
        {
            best_result.div = div;
            best_result.write_kbs = result.write_kbs;
            best_result.read_kbs = result.read_kbs;
            best_result.verify_errors = result.verify_errors;
            best_result.success = 1;
            best_div = div;
            found = 1;
        }
    }

    if (!found)
    {
        (void)SD_Card_InitAtClock(64U);
        LOG_Write("SD:SWEEP no reliable clock, fallback div=64\r\n");
        return 0;
    }

    (void)SD_Card_InitAtClock(best_div);
    LOG_Printf(
        "SD:SWEEP_BEST div=%lu write=%lu KB/s read=%lu KB/s\r\n",
        (unsigned long)best_result.div,
        (unsigned long)best_result.write_kbs,
        (unsigned long)best_result.read_kbs);

    if (best != NULL)
    {
        *best = best_result;
    }

    return 1;
}
