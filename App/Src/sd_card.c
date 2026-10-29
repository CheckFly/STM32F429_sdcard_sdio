#include "sd_card.h"

#include <string.h>

#include "app_config.h"
#include "log_uart.h"
#include "main.h"
#include "sd_partition.h"

SD_HandleTypeDef hsd;

static int SD_Card_InitInternal(uint32_t clock_div)
{
    hsd.Instance = SDIO;
    hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
    hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
    hsd.Init.ClockDiv = clock_div;

    LOG_Printf("SD:INIT_START div=%lu\r\n", (unsigned long)clock_div);

    if (HAL_SD_Init(&hsd) != HAL_OK)
    {
        LOG_Printf("SD:INIT_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
        (void)HAL_SD_DeInit(&hsd);
        return 0;
    }

    LOG_Write("SD:INIT_CARD_OK\r\n");

#if (SD_USE_4BIT != 0)
    /* The card must be initialized in 1-bit mode first, then switched to the
     * wide bus. */
    LOG_Write("SD:WIDE_START\r\n");
    if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK)
    {
        LOG_Printf("SD:WIDE_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
        (void)HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_1B);
        LOG_Write("SD:BUS 1-bit\r\n");
    }
    else
    {
        LOG_Write("SD:WIDE_OK\r\n");
        LOG_Write("SD:BUS 4-bit\r\n");
    }
#else
    LOG_Write("SD:BUS 1-bit\r\n");
#endif

    return 1;
}

static void SD_LogCardInfo(void)
{
    HAL_SD_CardInfoTypeDef card_info;

    if (HAL_SD_GetCardInfo(&hsd, &card_info) == HAL_OK)
    {
        SD_PartitionPlan plan;
        uint64_t bytes = (uint64_t)card_info.LogBlockNbr * (uint64_t)card_info.LogBlockSize;
        unsigned long capacity_mb = (unsigned long)(bytes / (1024ULL * 1024ULL));

        LOG_Printf(
            "SD:CARD type=%lu blocks=%lu block_size=%lu capacity=%luMB\r\n",
            (unsigned long)card_info.CardType,
            (unsigned long)card_info.LogBlockNbr,
            (unsigned long)card_info.LogBlockSize,
            capacity_mb);

        LOG_Printf("SD:CAPACITY total=%luMB\r\n", capacity_mb);

        if (SD_PartitionPlan_Compute(capacity_mb, 4096U, &plan))
        {
            LOG_Printf("SD:PART1 size=%luMB\r\n", (unsigned long)plan.part1_mb);
            LOG_Printf("SD:PART2 size=%luMB\r\n", (unsigned long)plan.part2_mb);
        }
        else
        {
            LOG_Write("SD:PARTITIONS unsupported (8GB+ only)\r\n");
        }
    }
}

int SD_Card_Init(void)
{
    if (!SD_Card_InitInternal(SD_CLOCK_DIV))
    {
        return 0;
    }

    LOG_Printf("SD:CLOCK div=%lu\r\n", (unsigned long)SD_CLOCK_DIV);
    SD_LogCardInfo();
    return 1;
}

int SD_Card_InitAtClock(uint32_t clock_div)
{
    (void)SD_Card_DeInit();
    HAL_Delay(10U);

    if (!SD_Card_InitInternal(clock_div))
    {
        return 0;
    }

    LOG_Printf("SD:CLOCK div=%lu\r\n", (unsigned long)clock_div);
    return 1;
}

int SD_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(&hsd);

        if (state == HAL_SD_CARD_TRANSFER)
        {
            return 1;
        }

        if (state == HAL_SD_CARD_ERROR || state == HAL_SD_CARD_DISCONNECTED)
        {
            return 0;
        }

        HAL_Delay(1U);
    }

    return 0;
}

int SD_WaitTransfer(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while (hsd.State == HAL_SD_STATE_BUSY)
    {
        if ((HAL_GetTick() - start) >= timeout_ms)
        {
            return 0;
        }
        HAL_Delay(1U);
    }

    return (hsd.ErrorCode == HAL_SD_ERROR_NONE) ? 1 : 0;
}

static void SD_MBR_SetU32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFU);
    buffer[1] = (uint8_t)((value >> 8) & 0xFFU);
    buffer[2] = (uint8_t)((value >> 16) & 0xFFU);
    buffer[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static void SD_MBR_FillEntry(
    uint8_t *entry,
    uint8_t status,
    uint8_t type,
    uint32_t start_lba,
    uint32_t sectors)
{
    entry[0] = status;
    entry[1] = 0xFEU;
    entry[2] = 0xFFU;
    entry[3] = 0xFFU;
    entry[4] = type;
    entry[5] = 0xFEU;
    entry[6] = 0xFFU;
    entry[7] = 0xFFU;
    SD_MBR_SetU32(&entry[8], start_lba);
    SD_MBR_SetU32(&entry[12], sectors);
}

int SD_CreatePartitions(uint32_t first_partition_mb)
{
    HAL_SD_CardInfoTypeDef card_info;
    SD_PartitionPlan plan;
    uint32_t total_mb;
    uint32_t part1_lba = 2048U; /* 1 MB alignment */
    uint32_t part1_sectors;
    uint32_t part2_lba;
    uint32_t part2_sectors;
    uint8_t part2_type;
    uint64_t bytes;
    uint32_t mbr[128];
    uint32_t verify[128];
    uint8_t *mbr_bytes = (uint8_t *)mbr;

    if (first_partition_mb == 0U)
    {
        first_partition_mb = 4096U;
    }

    if (HAL_SD_GetCardInfo(&hsd, &card_info) != HAL_OK)
    {
        LOG_Write("SD:CREATE_FAIL info\r\n");
        return 0;
    }

    bytes = (uint64_t)card_info.LogBlockNbr * (uint64_t)card_info.LogBlockSize;
    total_mb = (uint32_t)(bytes / (1024ULL * 1024ULL));

    if (!SD_PartitionPlan_Compute(total_mb, first_partition_mb, &plan))
    {
        LOG_Write("SD:CREATE_FAIL too small (8GB+ only)\r\n");
        return 0;
    }

    part1_sectors = plan.part1_mb * 2048U;
    part2_lba = part1_lba + part1_sectors;
    part2_sectors = plan.part2_mb * 2048U;
    part2_type = (plan.part2_mb > 32768U) ? 0x07U : 0x0CU;

    memset(mbr, 0, sizeof(mbr));
    SD_MBR_FillEntry(&mbr_bytes[446U], 0x00U, 0x0CU, part1_lba, part1_sectors);
    SD_MBR_FillEntry(&mbr_bytes[462U], 0x00U, part2_type, part2_lba, part2_sectors);
    mbr_bytes[510U] = 0x55U;
    mbr_bytes[511U] = 0xAAU;

    LOG_Write("SD:PARTITIONS_CREATE (erases partition table)\r\n");

    if (HAL_SD_WriteBlocks_DMA(&hsd, (uint8_t *)mbr, 0U, 1U) != HAL_OK)
    {
        LOG_Printf("SD:CREATE_WRITE_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
        return 0;
    }

    if (!SD_WaitTransfer(5000U))
    {
        LOG_Printf("SD:CREATE_WRITE_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
        return 0;
    }

    if (!SD_WaitReady(2000U))
    {
        LOG_Write("SD:CREATE_WAIT_TIMEOUT\r\n");
    }

    HAL_Delay(50U);

    {
        HAL_StatusTypeDef read_status = HAL_ERROR;
        uint32_t retry;

        for (retry = 0U; retry < 3U; ++retry)
        {
            read_status = HAL_SD_ReadBlocks_DMA(&hsd, (uint8_t *)verify, 0U, 1U);
            if (read_status == HAL_OK)
            {
                if (SD_WaitTransfer(5000U))
                {
                    break;
                }
                read_status = HAL_ERROR;
            }

            LOG_Printf("SD:CREATE_VERIFY_RETRY %lu err=%lu\r\n",
                       (unsigned long)retry, (unsigned long)hsd.ErrorCode);
            (void)SD_WaitReady(1000U);
            HAL_Delay(100U);
        }

        if (read_status != HAL_OK)
        {
            LOG_Printf("SD:CREATE_VERIFY_READ_FAIL err=%lu\r\n", (unsigned long)hsd.ErrorCode);
            return 0;
        }
    }

    if (memcmp(mbr, verify, sizeof(mbr)) != 0)
    {
        LOG_Write("SD:CREATE_VERIFY_MISMATCH\r\n");
        return 0;
    }

    LOG_Printf("SD:PART1 LBA=%lu sectors=%lu type=0x0C\r\n",
               (unsigned long)part1_lba, (unsigned long)part1_sectors);
    LOG_Printf("SD:PART2 LBA=%lu sectors=%lu type=0x%02X\r\n",
               (unsigned long)part2_lba, (unsigned long)part2_sectors, part2_type);
    LOG_Write("SD:PARTITIONS_CREATED\r\n");

    return 1;
}

void SD_Card_DeInit(void)
{
    (void)HAL_SD_DeInit(&hsd);
}
