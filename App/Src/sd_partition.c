#include "sd_partition.h"

#include <stddef.h>

#define SD_FIRST_PARTITION_DEFAULT_MB 4096U
#define SD_RESERVED_MB                1U
#define SD_MIN_SECOND_MB              64U

/* A marketed 8 GB card reports about 7,300-7,600 MiB, while a 4 GB card
 * reports about 3,600-3,800 MiB. This threshold accepts 8 GB and larger
 * cards and rejects smaller ones until a specific policy is defined. */
#define SD_MIN_TOTAL_MB               7000U

int SD_PartitionPlan_Compute(
    uint32_t total_mb,
    uint32_t first_partition_mb,
    SD_PartitionPlan *plan)
{
    uint32_t usable_mb;

    if (plan == NULL)
    {
        return 0;
    }

    if (first_partition_mb == 0U)
    {
        first_partition_mb = SD_FIRST_PARTITION_DEFAULT_MB;
    }

    plan->total_mb = total_mb;
    plan->part1_mb = 0U;
    plan->part2_mb = 0U;
    plan->valid = 0U;

    if (total_mb < SD_MIN_TOTAL_MB)
    {
        return 0;
    }

    usable_mb = total_mb - SD_RESERVED_MB;

    if (usable_mb < (first_partition_mb + SD_MIN_SECOND_MB))
    {
        return 0;
    }

    plan->part1_mb = first_partition_mb;
    plan->part2_mb = usable_mb - first_partition_mb;
    plan->valid = 1U;

    return 1;
}
