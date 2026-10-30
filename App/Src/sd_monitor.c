#include "sd_monitor.h"

#include "app_config.h"
#include "log_uart.h"
#include "main.h"
#include "sd_card.h"
#include "sd_fs.h"
#include "sd_test.h"

#define SD_DEBOUNCE_MS 200U
#define SD_RETRY_MS    2000U

/* Hardware conflict: PC10 is LTDC_R2 on the on-board LCD. Keep SDIO disabled
 * until the module's D2/D3 lines are physically disconnected and the 1-bit
 * wiring has been rechecked. Set to 1 only after the hardware fix. */
#ifndef SD_ENABLE_SDIO
#define SD_ENABLE_SDIO 1
#endif

static uint8_t stable_present;
static uint8_t raw_last;
static uint32_t raw_change_ms;
static uint8_t sd_ready;
static uint8_t fail_logged;
static uint32_t next_retry_ms;

static void SD_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = SD_CARD_DET_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SD_CARD_DET_GPIO_Port, &gpio);
}

static uint8_t SD_ReadPresent(void)
{
    /* Active-high card-detect signal: high = card inserted. */
    return (HAL_GPIO_ReadPin(SD_CARD_DET_GPIO_Port, SD_CARD_DET_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static void SD_TryInit(uint32_t now)
{
#if SD_ENABLE_SDIO
    if (sd_ready)
    {
        return;
    }

    if ((int32_t)(now - next_retry_ms) < 0)
    {
        return;
    }

    if (SD_Card_Init())
    {
        sd_ready = 1U;
        fail_logged = 0U;
        LOG_Write("SD:INIT_OK\r\n");

        {
            uint8_t can_test = 1U;

#if SD_CREATE_PARTITIONS_ON_INIT
            LOG_Write("SD:PARTITIONS_CREATE requested\r\n");
            if (SD_CreatePartitions(4096U))
            {
                LOG_Write("SD:PARTITIONS_OK\r\n");
            }
            else
            {
                can_test = 0U;
                LOG_Write("SD:TEST_SKIPPED (partitions failed)\r\n");
            }
#endif

            /* Mount after the partition table is in place: the diskio layer
             * locates the FAT partition by reading the MBR. Never format when
             * the partition table could not be validated. */
#if SD_MOUNT_FS_ON_INIT
            if (!SD_FS_Mount((can_test != 0U) ? SD_FORMAT_PART2_ON_INIT : 0))
            {
                LOG_Write("SD:FS_NOT_MOUNTED\r\n");
            }
#if SD_FS_SELFTEST_ON_INIT
            else
            {
                (void)SD_FS_SelfTest();
            }
#endif
#endif

#if SD_RUN_SPEED_TEST_ON_INIT
            if (can_test)
            {
                SD_SpeedTestResult test;
                /* Test at the start of partition 2 (after the 4 GB partition). */
                const uint32_t test_lba = 2048U + (4096U * 2048U);
                (void)SD_TestReadWrite(test_lba, SD_SPEED_TEST_BLOCKS, &test);
            }
#else
            (void)can_test;
#endif

#if SD_RUN_CLOCK_SWEEP_ON_INIT
            {
                SD_ClockResult best;
                (void)SD_TestClockSweep(2048U, 1024U, &best);
            }
#endif
        }
    }
    else
    {
        if (!fail_logged)
        {
            LOG_Write("SD:INIT_FAIL\r\n");
            fail_logged = 1U;
        }
        next_retry_ms = now + SD_RETRY_MS;
    }
#else
    (void)now;
    (void)next_retry_ms;
    (void)fail_logged;
#endif
}

static void SD_HandleInserted(uint32_t now)
{
    LOG_Write("SD:PRESENT\r\n");
    next_retry_ms = 0U;
    fail_logged = 0U;
    SD_TryInit(now);
}

static void SD_HandleRemoved(void)
{
    if (sd_ready)
    {
        SD_Card_DeInit();
        sd_ready = 0U;
    }

    fail_logged = 0U;
    LOG_Write("SD:REMOVED\r\n");
}

void SD_Monitor_Init(void)
{
    SD_GPIO_Init();

    stable_present = SD_ReadPresent();
    raw_last = stable_present;
    raw_change_ms = HAL_GetTick();
    sd_ready = 0U;
    next_retry_ms = 0U;

    if (stable_present)
    {
        SD_HandleInserted(HAL_GetTick());
    }
    else
    {
        LOG_Write("SD:REMOVED\r\n");
    }
}

void SD_Monitor_Process(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t raw = SD_ReadPresent();

    if (raw != raw_last)
    {
        raw_last = raw;
        raw_change_ms = now;
        return;
    }

    if ((raw != stable_present) && ((now - raw_change_ms) >= SD_DEBOUNCE_MS))
    {
        stable_present = raw;

        if (stable_present)
        {
            SD_HandleInserted(now);
        }
        else
        {
            SD_HandleRemoved();
        }
    }

    if (stable_present)
    {
        SD_TryInit(now);
    }
}

int SD_Monitor_IsPresent(void)
{
    return stable_present ? 1 : 0;
}
