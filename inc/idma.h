#ifndef _IDMA_H_
#define _IDMA_H_

#include <iommu_tests.h>

// Number of DMA devices in the platform
#define N_DMA           (8)

#define IDMA_DECOUPLE   (1ULL << 0)
#define IDMA_DEBURST    (1ULL << 1)
#define IDMA_SERIALIZE  (1ULL << 2)

// Base address of the iDMA programming interface
#define IDMA_BASE_ADDR             0x50000000ULL

// Register offsets
#define IDMA_SRC_ADDR_OFF   0x0
#define IDMA_DEST_ADDR_OFF  0x8
#define IDMA_N_BYTES_OFF    0x10
#define IDMA_CONFIG_OFF     0x18
#define IDMA_STATUS_OFF     0x20
#define IDMA_NEXT_ID_OFF    0x28
#define IDMA_DONE_OFF       0x30
#define IDMA_IPSR_OFF       0x38

#define IDMA_REG_ADDR(INDEX, OFF)      (IDMA_BASE_ADDR + (INDEX * 0x1000ULL) + OFF)

void idma_setup(int idx, uint64_t src, uint64_t dst, uint64_t n_bytes);
void idma_setup_addr(int idx, uint64_t src, uint64_t dst);
int idma_exec_transfer(int idx);

#endif  /* _IDMA_H_ */