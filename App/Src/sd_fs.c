#include "sd_fs.h"

#include <string.h>

#include "ff.h"
#include "ff_gen_drv.h"
#include "log_uart.h"
#include "sd_diskio.h"

#define SD_FS_WORK_SIZE 4096U

static FATFS sd_fatfs;
static char sd_fs_path[4];
static int sd_fs_linked;
static int sd_fs_mounted;

/* f_mkfs() work area: must be at least one sector, 4 KB is comfortable. */
static uint8_t sd_fs_work[SD_FS_WORK_SIZE] __attribute__((aligned(32)));

int SD_FS_Mount(int format_if_missing)
{
    FRESULT result;
    uint32_t base_lba = 0U;
    uint32_t sectors = 0U;

    if (sd_fs_mounted)
    {
        return 1;
    }

    if (!sd_fs_linked)
    {
        if (FATFS_LinkDriver(&SD_Driver, sd_fs_path) != 0)
        {
            LOG_Write("SD:FS_LINK_FAIL\r\n");
            return 0;
        }
        sd_fs_linked = 1;
    }

    result = f_mount(&sd_fatfs, sd_fs_path, 1);

    if (result == FR_NO_FILESYSTEM && format_if_missing)
    {
        LOG_Write("SD:FS_FORMAT_START (erases partition 2)\r\n");

        result = f_mkfs(sd_fs_path, FM_FAT32, 0U, sd_fs_work, sizeof(sd_fs_work));
        if (result != FR_OK)
        {
            LOG_Printf("SD:FS_FORMAT_FAIL res=%u\r\n", (unsigned)result);
            (void)f_mount(NULL, sd_fs_path, 0);
            return 0;
        }

        LOG_Write("SD:FS_FORMAT_OK\r\n");

        /* f_mkfs() leaves the volume unmounted: mount the fresh filesystem. */
        result = f_mount(&sd_fatfs, sd_fs_path, 1);
    }

    if (result != FR_OK)
    {
        LOG_Printf("SD:FS_MOUNT_FAIL res=%u\r\n", (unsigned)result);
        (void)f_mount(NULL, sd_fs_path, 0);
        FATFS_UnLinkDriver(sd_fs_path);
        sd_fs_linked = 0;
        return 0;
    }

    sd_fs_mounted = 1;

    if (SD_Diskio_PartitionInfo(&base_lba, &sectors))
    {
        LOG_Printf(
            "SD:FS_MOUNT_OK drive=%s lba=%lu sectors=%lu\r\n",
            sd_fs_path,
            (unsigned long)base_lba,
            (unsigned long)sectors);
    }
    else
    {
        LOG_Printf("SD:FS_MOUNT_OK drive=%s\r\n", sd_fs_path);
    }

    SD_FS_LogUsage();
    return 1;
}

void SD_FS_Unmount(void)
{
    if (!sd_fs_mounted)
    {
        return;
    }

    (void)f_mount(NULL, sd_fs_path, 0);
    FATFS_UnLinkDriver(sd_fs_path);
    sd_fs_linked = 0;
    sd_fs_mounted = 0;
}

const char *SD_FS_Path(void)
{
    return sd_fs_mounted ? sd_fs_path : NULL;
}

int SD_FS_IsMounted(void)
{
    return sd_fs_mounted;
}

void SD_FS_LogUsage(void)
{
    FATFS *fs = NULL;
    DWORD free_clusters = 0U;
    FRESULT result;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint32_t cluster_bytes;

    if (!sd_fs_mounted)
    {
        return;
    }

    result = f_getfree(sd_fs_path, &free_clusters, &fs);
    if (result != FR_OK || fs == NULL)
    {
        LOG_Printf("SD:FS_USAGE_FAIL res=%u\r\n", (unsigned)result);
        return;
    }

    cluster_bytes = (uint32_t)fs->csize * 512U;
    total_bytes = (uint64_t)(fs->n_fatent - 2U) * cluster_bytes;
    free_bytes = (uint64_t)free_clusters * cluster_bytes;

    LOG_Printf(
        "SD:FS_USAGE total=%luMB free=%luMB cluster=%luB\r\n",
        (unsigned long)(total_bytes / (1024ULL * 1024ULL)),
        (unsigned long)(free_bytes / (1024ULL * 1024ULL)),
        (unsigned long)cluster_bytes);
}

void SD_FS_LogRoot(void){
    DIR dir;
    FILINFO info;
    FRESULT result;
    uint32_t count = 0U;

    if (!sd_fs_mounted)
    {
        return;
    }

    result = f_opendir(&dir, sd_fs_path);
    if (result != FR_OK)
    {
        LOG_Printf("SD:FS_OPENDIR_FAIL res=%u\r\n", (unsigned)result);
        return;
    }

    for (;;)
    {
        result = f_readdir(&dir, &info);
        if (result != FR_OK || info.fname[0] == '\0')
        {
            break;
        }

        LOG_Printf(
            "SD:FS_ENTRY name=%s size=%lu attr=0x%02X\r\n",
            info.fname,
            (unsigned long)info.fsize,
            (unsigned)info.fattrib);

        if (++count >= 32U)
        {
            break;
        }
    }

    (void)f_closedir(&dir);
    LOG_Printf("SD:FS_ROOT entries=%lu\r\n", (unsigned long)count);
}

int SD_FS_SelfTest(void)
{
    static const char pattern[] = "F429 SDIO 4-bit FatFs selftest 0123456789";
    FIL file;
    FRESULT result;
    UINT written = 0U;
    UINT read = 0U;
    char buffer[sizeof(pattern)];
    int ok = 0;

    if (!sd_fs_mounted)
    {
        return 0;
    }

    LOG_Write("SD:FS_TEST_START\r\n");

    result = f_open(&file, "0:/FSTEST.TXT", FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK)
    {
        LOG_Printf("SD:FS_TEST_OPEN_W_FAIL res=%u\r\n", (unsigned)result);
        return 0;
    }

    result = f_write(&file, pattern, (UINT)(sizeof(pattern) - 1U), &written);
    (void)f_close(&file);

    if (result != FR_OK || written != (sizeof(pattern) - 1U))
    {
        LOG_Printf("SD:FS_TEST_WRITE_FAIL res=%u n=%u\r\n", (unsigned)result, (unsigned)written);
        return 0;
    }

    result = f_open(&file, "0:/FSTEST.TXT", FA_READ);
    if (result != FR_OK)
    {
        LOG_Printf("SD:FS_TEST_OPEN_R_FAIL res=%u\r\n", (unsigned)result);
        return 0;
    }

    result = f_read(&file, buffer, (UINT)(sizeof(pattern) - 1U), &read);
    (void)f_close(&file);

    if (result != FR_OK || read != (sizeof(pattern) - 1U) ||
        memcmp(buffer, pattern, sizeof(pattern) - 1U) != 0)
    {
        LOG_Printf("SD:FS_TEST_READ_FAIL res=%u n=%u\r\n", (unsigned)result, (unsigned)read);
        return 0;
    }

    /* The file is only a probe: keep the partition clean. */
    if (f_unlink("0:/FSTEST.TXT") == FR_OK)
    {
        ok = 1;
    }

    LOG_Printf("SD:FS_TEST %s bytes=%u\r\n", ok ? "OK" : "UNLINK_FAIL",
               (unsigned)(sizeof(pattern) - 1U));

    return ok;
}
