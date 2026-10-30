#ifndef APP_SD_CARD_H
#define APP_SD_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/** Global SD handle, managed by App/Src/sd_card.c. */
extern SD_HandleTypeDef hsd;

/** Initialize SDIO in 1-bit mode and print the card information. */
int SD_Card_Init(void);

/** Re-initialize SDIO with a specific clock divider. Returns 1 on success. */
int SD_Card_InitAtClock(uint32_t clock_div);

/** De-initialize SDIO and release the card. */
void SD_Card_DeInit(void);

/** Wait until the card returns to the transfer state. Returns 1 on success. */
int SD_WaitReady(uint32_t timeout_ms);

/** Wait for an asynchronous DMA transfer to finish. Returns 1 on success. */
int SD_WaitTransfer(uint32_t timeout_ms);

/**
 * @brief Create an MBR with two partitions on the SD card.
 *
 * Partition 1: first_partition_mb (default 4096 MB) starting at 1 MB,
 *              type 0xDA (non-filesystem, internal looper area).
 * Partition 2: remaining space minus 1 MB, starting after partition 1,
 *              FAT32 LBA (or exFAT type for very large volumes).
 * The card is NOT formatted; only the partition table is written.
 * This operation erases the existing partition table.
 *
 * @return 1 on success, 0 on error.
 */
int SD_CreatePartitions(uint32_t first_partition_mb);

#ifdef __cplusplus
}
#endif

#endif /* APP_SD_CARD_H */
