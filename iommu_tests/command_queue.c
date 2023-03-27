/**
 * !NOTES:
 * 
 *  -   
 */

#include <iommu_tests/command_queue.h>

// N_entries * 16 bytes
uint64_t command_queue[CQ_N_ENTRIES * 2 * sizeof(uint64_t)] __attribute__((aligned(PAGE_SIZE)));

void cq_init()
{
    uint64_t    cqb;
    uintptr_t   cqb_addr    = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
    uintptr_t   cqt_addr    = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);
    uintptr_t   cqcsr_addr  = IOMMU_REG_ADDR(IOMMU_CQCSR_OFFSET);

    // Configure cqb with base PPN of the queue and size as log2(N)
    cqb = ((((uintptr_t)command_queue) >> 2) & CQB_PPN_MASK) | CQ_LOG2SZ_1;
    write64(cqb_addr, cqb);

    // Set cqt to zero
    write64(cqt_addr, 0);

    // Write 1 to cqcsr.cqen to enable the CQ
    // INFO: Interrupts disabled
    write64(cqcsr_addr, 1);

    // Poll cqcsr.cqon until it reads 1
    while (!(read64(cqcsr_addr) & CQCSR_CQON));
}

