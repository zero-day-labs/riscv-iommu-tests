#ifndef _IDMA_H_
#define _IDMA_H_

#include <rv_iommu_tests.h>

#define IDMA_DECOUPLE   (1ULL << 0)
#define IDMA_DEBURST    (1ULL << 1)
#define IDMA_SERIALIZE  (1ULL << 2)

struct idma {
  uint64_t src_addr;
  uint64_t dest_addr;
  uint64_t num_bytes;
  uint64_t config;
  uint64_t status;
  uint64_t next_transfer_id;
  uint64_t last_transfer_id_complete;
  uint64_t ipsr;
}__attribute__((__packed__, aligned(PAGE_SIZE)));

void idma_setup(struct idma *dma_ut, uint64_t src, uint64_t dst, uint64_t n_bytes);
void idma_setup_addr(struct idma *dma_ut, uint64_t src, uint64_t dst);
int idma_exec_transfer(struct idma *dma_ut);

#endif  /* _IDMA_H_ */