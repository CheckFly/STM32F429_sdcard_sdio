#ifndef APP_SD_FS_H
#define APP_SD_FS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mount the user data partition (partition 2) as FatFs drive "0:".
 *
 * Partition 1 is the internal raw area used by the looper; it is not exposed
 * to FatFs and stays invisible on a PC.
 *
 * @param format_if_missing 1 to create a FAT32 volume when no filesystem is
 *                          found. This erases partition 2.
 * @return 1 on success, 0 on failure.
 */
int SD_FS_Mount(int format_if_missing);

/** Unmount the drive and release the FatFs link. */
void SD_FS_Unmount(void);

/** FatFs path of the mounted volume ("0:"), or NULL when not mounted. */
const char *SD_FS_Path(void);

/** 1 when the volume is mounted. */
int SD_FS_IsMounted(void);

/** Log free/total space of the mounted volume. */
void SD_FS_LogUsage(void);

/** Log the root directory entries (debug helper). */
void SD_FS_LogRoot(void);

/**
 * @brief Create, write, read back and delete a small file.
 * @return 1 on success, 0 on failure.
 */
int SD_FS_SelfTest(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SD_FS_H */
