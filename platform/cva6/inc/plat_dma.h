#ifndef _IOMMU_H_
#define _IOMMU_H_

#include <stdint.h>

#define N_DMA           (1)
extern uint64_t idma_ids[N_DMA];
extern uint64_t idma_addr[N_DMA];

#endif /*_IOMMU_H_*/