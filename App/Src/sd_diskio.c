#include "sd_diskio.h"

#include <string.h>

#include "log_uart.h"
#include "sd_card.h"

#define SD_SECTOR_SIZE        512U
#define SD_TRANSFER_TIMEOUT   5000U

/* Partition entry type values that can hold a FAT/exFAT filesystem. */
static int SD_Diskio_IsFatType(uint8_t type)
{
    switch (type)
    {
        case 0x01U: /* FAT12 */
        case 0x04U: /* FAT16 < 32 MB */
        case 0x06U: /* FAT16 */
        case 0x0BU: /* FAT32 CHS */
        case 0x0CU: /* FAT32 LBA */
        case 0x07U: /* exFAT / NTFS */
        case 0x0EU: /* FAT16 LBA */
            return 1;
        default:
            return 0;
    }
}

static uint32_t SD_Diskio_ReadU32(const uint8_t *buffer)
{
    return (uint32_t)buffer[0]
         | ((uint32_t)buffer[1] << 8U)
         | ((uint32_t)buffer[2] << 16U)
         | ((uint32_t)buffer[3] << 24U);
}

/* Bounce buffer for transfers whose caller buffer is not word aligned. */
static uint8_t sd_disk_bounce[SD_SECTOR_SIZE] __attribute__((aligned(32)));
static uint8_t sd_disk_mbr[SD_SECTOR_SIZE] __attribute__((aligned(32)));

static DSTATUS sd_disk_status = STA_NOINIT;
static uint32_t sd_disk_base_lba;
static uint32_t sd_disk_sectors;

static int SD_Diskio_RawRead(uint32_t lba, uint8_t *dst, uint32_t blocks)
{
    uint32_t attempt;

    for (attempt = 0U; attempt < 3U; ++attempt)
    {
        if (attempt > 0U)
        {
            (void)SD_WaitReady(1000U);
            HAL_Delay(10U);
        }

        if (HAL_SD_ReadBlocks_DMA(&hsd, dst, lba, blocks) == HAL_OK && SD_WaitTransfer(SD_TRANSFER_TIMEOUT))
        {
            return 1;
        }
    }

    LOG_Printf("SD:FS_READ_FAIL lba=%lu n=%lu err=%lu\r\n",
               (unsigned long)lba,
               (unsigned long)blocks,
               (unsigned long)hsd.ErrorCode);
    return 0;
}

static int SD_Diskio_RawWrite(uint32_t lba, const uint8_t *src, uint32_t blocks)
{
    uint32_t attempt;

    for (attempt = 0U; attempt < 3U; ++attempt)
    {
        if (attempt > 0U)
        {
            (void)SD_WaitReady(1000U);
            HAL_Delay(10U);
        }

        if (HAL_SD_WriteBlocks_DMA(&hsd, (uint8_t *)src, lba, blocks) == HAL_OK && SD_WaitTransfer(SD_TRANSFER_TIMEOUT))
        {
            return 1;
        }
    }

    LOG_Printf("SD:FS_WRITE_FAIL lba=%lu n=%lu err=%lu\r\n",
               (unsigned long)lba,
               (unsigned long)blocks,
               (unsigned long)hsd.ErrorCode);
    return 0;
}

/**
 * Read the MBR and locate the FAT partition that FatFs should expose.
 * Partition 1 is the internal raw area (non-FAT type) and is deliberately
 * skipped: only the user-visible data partition is mounted.
 *
 * If several FAT entries exist (older layout where partition 1 was also
 * FAT32), the last one wins: the data partition is always the highest one.
 */
static int SD_Diskio_FindPartition(void)
{
    uint32_t index;
    uint32_t found_lba = 0U;
    uint32_t found_sectors = 0U;

    for (index = 0U; index < 4U; ++index)
    {
        const uint8_t *entry = &sd_disk_mbr[446U + (index * 16U)];
        uint8_t type = entry[4U];
        uint32_t lba;
        uint32_t sectors;

        if (!SD_Diskio_IsFatType(type))
        {
            continue;
        }

        lba = SD_Diskio_ReadU32(&entry[8U]);
        sectors = SD_Diskio_ReadU32(&entry[12U]);

        if (lba != 0U && sectors != 0U)
        {
            found_lba = lba;
            found_sectors = sectors;
        }
    }

    if (found_lba == 0U || found_sectors == 0U)
    {
        return 0;
    }

    sd_disk_base_lba = found_lba;
    sd_disk_sectors = found_sectors;
    return 1;
}

static DSTATUS SD_Diskio_Init(BYTE lun)
{
    uint32_t attempt;

    (void)lun;

    sd_disk_status = STA_NOINIT;
    sd_disk_base_lba = 0U;
    sd_disk_sectors = 0U;

    /* The first transfer after a write/wide-bus switch can return stale data,
     * so validate the MBR and retry before declaring the drive unusable. */
    for (attempt = 0U; attempt < 3U; ++attempt)
    {
        if (attempt > 0U)
        {
            (void)SD_WaitReady(1000U);
            HAL_Delay(50U);
        }

        if (!SD_Diskio_RawRead(0U, sd_disk_mbr, 1U))
        {
            continue;
        }

        if (sd_disk_mbr[510U] != 0x55U || sd_disk_mbr[511U] != 0xAAU)
        {
            LOG_Printf("SD:FS_MBR_BAD sig=%02X%02X\r\n",
                       (unsigned)sd_disk_mbr[510U],
                       (unsigned)sd_disk_mbr[511U]);
            continue;
        }

        if (!SD_Diskio_FindPartition())
        {
            LOG_Printf("SD:FS_MBR_NOFAT t1=%02X t2=%02X\r\n",
                       (unsigned)sd_disk_mbr[446U + 4U],
                       (unsigned)sd_disk_mbr[462U + 4U]);
            continue;
        }

        sd_disk_status = 0U;
        break;
    }

    return sd_disk_status;
}

static DSTATUS SD_Diskio_Status(BYTE lun)
{
    (void)lun;
    return sd_disk_status;
}

static DRESULT SD_Diskio_Read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
    (void)lun;

    if ((sd_disk_status & STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }

    if (buff == NULL || count == 0U)
    {
        return RES_PARERR;
    }

    /* Out-of-partition access is a caller bug: never touch another area. */
    if ((sector + count) > sd_disk_sectors)
    {
        return RES_PARERR;
    }

    if (((uint32_t)buff & 3U) == 0U)
    {
        return SD_Diskio_RawRead(sd_disk_base_lba + sector, buff, count) ? RES_OK : RES_ERROR;
    }

    {
        UINT index;

        for (index = 0U; index < count; ++index)
        {
            if (!SD_Diskio_RawRead(sd_disk_base_lba + sector + index, sd_disk_bounce, 1U))
            {
                return RES_ERROR;
            }
            memcpy(&buff[index * SD_SECTOR_SIZE], sd_disk_bounce, SD_SECTOR_SIZE);
        }
    }

    return RES_OK;
}

static DRESULT SD_Diskio_Write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
    (void)lun;

    if ((sd_disk_status & STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }

    if (buff == NULL || count == 0U)
    {
        return RES_PARERR;
    }

    if ((sector + count) > sd_disk_sectors)
    {
        return RES_PARERR;
    }

    if (((uint32_t)buff & 3U) == 0U)
    {
        return SD_Diskio_RawWrite(sd_disk_base_lba + sector, buff, count) ? RES_OK : RES_ERROR;
    }

    {
        UINT index;

        for (index = 0U; index < count; ++index)
        {
            memcpy(sd_disk_bounce, &buff[index * SD_SECTOR_SIZE], SD_SECTOR_SIZE);
            if (!SD_Diskio_RawWrite(sd_disk_base_lba + sector + index, sd_disk_bounce, 1U))
            {
                return RES_ERROR;
            }
        }
    }

    return RES_OK;
}

static DRESULT SD_Diskio_Ioctl(BYTE lun, BYTE cmd, void *buff)
{
    (void)lun;

    if ((sd_disk_status & STA_NOINIT) != 0U)
    {
        return RES_NOTRDY;
    }

    switch (cmd)
    {
        case CTRL_SYNC:
            return SD_WaitReady(SD_TRANSFER_TIMEOUT) ? RES_OK : RES_ERROR;

        case GET_SECTOR_COUNT:
            *(DWORD *)buff = sd_disk_sectors;
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD *)buff = SD_SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1U;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

Diskio_drvTypeDef SD_Driver =
{
    SD_Diskio_Init,
    SD_Diskio_Status,
    SD_Diskio_Read,
    SD_Diskio_Write,
    SD_Diskio_Ioctl,
};

int SD_Diskio_PartitionInfo(uint32_t *base_lba, uint32_t *sectors)
{
    if ((sd_disk_status & STA_NOINIT) != 0U)
    {
        return 0;
    }

    if (base_lba != NULL)
    {
        *base_lba = sd_disk_base_lba;
    }

    if (sectors != NULL)
    {
        *sectors = sd_disk_sectors;
    }

    return 1;
}
