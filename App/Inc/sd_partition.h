#ifndef APP_SD_PARTITION_H
#define APP_SD_PARTITION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
    uint32_t total_mb;
    uint32_t part1_mb;
    uint32_t part2_mb;
    uint8_t valid;
} SD_PartitionPlan;

/**
 * @brief Compute a two-partition plan from the card's real capacity.
 *
 * Supported cards: 8 GB and larger. A marketed 8 GB card reports roughly
 * 7,300-7,600 MiB, so the threshold is 7,000 MiB. Partition 1 is
 * first_partition_mb (4096 MiB by default); partition 2 gets the remaining
 * space minus 1 MiB reserved for the MBR and alignment. Smaller cards are
 * not partitioned yet.
 *
 * @return 1 when a plan is produced, 0 when the card is below 8 GB.
 */
int SD_PartitionPlan_Compute(
    uint32_t total_mb,
    uint32_t first_partition_mb,
    SD_PartitionPlan *plan);

#ifdef __cplusplus
}
#endif

#endif /* APP_SD_PARTITION_H */
