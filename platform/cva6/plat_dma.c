
#include "plat_dma.h"

/**
 *  iDMA device IDs 
 *  (should be populated with the hardwired value)
 */
uint64_t idma_ids[N_DMA] = {
    10ULL
};

/**
 *  iDMA device base addresses
 *  We assume that DMA devices may not be contigous in memory
 */
uint64_t idma_addr[N_DMA] = {
    0x50000000ULL
};