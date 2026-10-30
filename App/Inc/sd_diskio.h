#ifndef APP_SD_DISKIO_H
#define APP_SD_DISKIO_H

#include "ff_gen_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/** FatFs driver entry, bound to the data partition (partition 2). */
extern Diskio_drvTypeDef SD_Driver;

/** Start LBA and size (in 512-byte sectors) of the FatFs partition. */
int SD_Diskio_PartitionInfo(uint32_t *base_lba, uint32_t *sectors);

#ifdef __cplusplus
}
#endif

#endif /* APP_SD_DISKIO_H */
