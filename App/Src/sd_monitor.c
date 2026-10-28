#include "sd_monitor.h"

#include "log_uart.h"
#include "main.h"

#define SD_DEBOUNCE_MS 200U

static uint8_t stable_present;
static uint8_t raw_last;
static uint32_t raw_change_ms;

static uint8_t SD_ReadPresent(void)
{
    /* Active-low card-detect switch: low = card inserted. */
    return (HAL_GPIO_ReadPin(SD_CARD_DET_GPIO_Port, SD_CARD_DET_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

void SD_Monitor_Init(void)
{
    stable_present = SD_ReadPresent();
    raw_last = stable_present;
    raw_change_ms = HAL_GetTick();

    if (stable_present)
    {
        LOG_Write("SD:PRESENT\r\n");
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
            LOG_Write("SD:PRESENT\r\n");
            /* TODO: mount FatFs here and report SD:MOUNT_OK, SD:PART1_OK, SD:PART2_OK. */
        }
        else
        {
            LOG_Write("SD:REMOVED\r\n");
            /* TODO: unmount FatFs here. */
        }
    }
}

int SD_Monitor_IsPresent(void)
{
    return stable_present ? 1 : 0;
}
