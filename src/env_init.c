// iommu_init.c
#include <stdint.h>

// -----------------------------------------------------------------------------
// MMIO helpers
// -----------------------------------------------------------------------------
#define MMIO64(addr) (*(volatile uint64_t *)(addr))
#define MMIO32(addr) (*(volatile uint32_t *)(addr))

// -----------------------------------------------------------------------------
// Constants (same as before)
// -----------------------------------------------------------------------------
#define S1PT_BASE         0x10002000ULL
#define S2PT_ROOT_BASE    0x10003000ULL
#define S2PT_BASE         0x10005000ULL

#define PAGE_SIZE_4K      0x1000ULL
#define TEST_VPAGE_BASE   0x00000000ULL
#define TEST_PPAGE_BASE   0x1000F000ULL

#define TEST_PAGE_MAX     512

#define PTE_V             (1ULL << 0)
#define PTE_U             (1ULL << 4)
#define PTE_RWX           ((1ULL<<1)|(1ULL<<2)|(1ULL<<3))
#define PTE_AD            ((3ULL << 6))
#define PTE_PPN_MSK       0x3FFFFFFFFFFC00ULL

#define ROOT_DDT_BASE     0x0ULL
#define ROOT_DDT_SIZE     0x40U
#define DDT_N_ENTRIES     64

#define OFF_TC            0x00
#define OFF_IOHGATP       0x08
#define OFF_FSC           0x18

#define IOMMU_DDTP_ADDR   0x10
#define IOMMU_PQB_ADDR    0x38
#define IOMMU_PQH_ADDR    0x40
#define IOMMU_PQT_ADDR    0x44
#define IOMMU_PQCSR_ADDR  0x50
#define IOMMU_CQB_ADDR    0x18
#define IOMMU_CQH_ADDR    0x20
#define IOMMU_CQT_ADDR    0x24
#define IOMMU_CQCSR_ADDR  0x48

#define IOHGATP_MODE_SV39X4 0x8ULL
#define IOSATP_MODE_SV39    0x8ULL
#define DDTP_MODE_1LVL      0x2ULL

#define PQB_PPN_MASK      0x3FFFFFFFFFFC00ULL
#define PQ_LOG2SZ_1       0x2ULL
#define PQCSR_CQEN        0x1U
#define PQCSR_CIE         0x2U
#define PQCSR_CQON        0x00010000U

#define CQB_PPN_MASK      0x3FFFFFFFFFFC00ULL
#define CQ_LOG2SZ_1       0x1ULL
#define CQCSR_CQEN        0x1U
#define CQCSR_CIE         0x2U
#define CQCSR_CQON        0x00010000U

// -----------------------------------------------------------------------------
// S1PT initialization: three-level Stage-1 page table
// -----------------------------------------------------------------------------
void s1pt_init_hw(void) {
    uint64_t base_ptr, pte_val;
    uint64_t phys = TEST_VPAGE_BASE + 0x00080000ULL;

    // 1) Clear levels 0,1,2
    for (int lvl = 0; lvl < 3; lvl++) {
        for (int i = 0; i < 512; i++) {
            MMIO64(S1PT_BASE + ((lvl*512 + i)*8)) = 0ULL;
        }
    }

    // 2) Link level-1 & level-2 into level-0
    base_ptr = S1PT_BASE + (1*512 + 0)*8;
    pte_val  = PTE_V | ((base_ptr >> 2) & PTE_PPN_MSK);
    MMIO64(S1PT_BASE + (0*512 + 0)*8) = pte_val;
    MMIO64(S1PT_BASE + (0*512 + 4)*8) = pte_val;
    MMIO64(S1PT_BASE + (0*512 + 8)*8) = pte_val;

    base_ptr = S1PT_BASE + (2*512 + 0)*8;
    pte_val  = PTE_V | ((base_ptr >> 2) & PTE_PPN_MSK);
    MMIO64(S1PT_BASE + (1*512 + 0)*8) = pte_val;
    MMIO64(S1PT_BASE + (1*512 + 4)*8) = pte_val;
    MMIO64(S1PT_BASE + (1*512 + 511)*8) = pte_val;

    // 3) Populate leaf entries
    for (int i = 0; i < TEST_PAGE_MAX; i++, phys += PAGE_SIZE_4K) {
        pte_val = (phys >> 2) | PTE_AD | PTE_V | PTE_U | PTE_RWX;
        MMIO64(S1PT_BASE + (2*512 + i)*8) = pte_val;
    }
}

// -----------------------------------------------------------------------------
// S2PT initialization: two-level Stage-2 page table
// -----------------------------------------------------------------------------
void s2pt_init_hw(void) {
    uint64_t base_ptr, pte_val;
    uint64_t phys = TEST_PPAGE_BASE;

    // 1) Clear root & first leaf
    for (int i = 0; i < 2048; i++)
        MMIO64(S2PT_ROOT_BASE + i*8) = 0ULL;
    for (int i = 0; i < 512; i++)
        MMIO64(S2PT_BASE + (1*512 + i)*8) = 0ULL;

    // 2) Link entries
    base_ptr = S2PT_ROOT_BASE + 1*8;
    pte_val  = PTE_V | ((base_ptr >> 2) & PTE_PPN_MSK);
    MMIO64(S2PT_ROOT_BASE + 4*8)    = pte_val;
    MMIO64(S2PT_ROOT_BASE + 2047*8) = pte_val;

    base_ptr = S2PT_BASE + 0;
    pte_val  = PTE_V | ((base_ptr >> 2) & PTE_PPN_MSK);
    MMIO64(S2PT_ROOT_BASE + 2*8) = pte_val;

    // 3) Populate leaf entries
    for (int i = 0; i < TEST_PAGE_MAX; i++, phys += PAGE_SIZE_4K) {
        pte_val = (phys >> 2) | PTE_AD | PTE_V | PTE_U | PTE_RWX;
        MMIO64(S2PT_BASE + (1*512 + i)*8) = pte_val;
    }
}

// -----------------------------------------------------------------------------
// DDT init: zero entries & program first three in 1-level mode
// -----------------------------------------------------------------------------
void iommu_ddt_init_hw(void) {
    uint64_t entry, val64, ddtp;

    // Zero all DDT entries
    for (int i = 0; i < DDT_N_ENTRIES; i++) {
        entry = ROOT_DDT_BASE + i*ROOT_DDT_SIZE;
        for (int off = 0; off < ROOT_DDT_SIZE; off += 8)
            MMIO64(entry + off) = 0ULL;
    }

    // Program tc, iohgatp, fsc
    for (int i = 0; i <= 2; i++) {
        entry = ROOT_DDT_BASE + i*ROOT_DDT_SIZE;
        MMIO32(entry + OFF_TC) = 0x3;
        val64 = (IOHGATP_MODE_SV39X4 << 6) | (S2PT_ROOT_BASE >> 12);
        MMIO64(entry + OFF_IOHGATP) = val64;
        val64 = (IOSATP_MODE_SV39 << 6) | (S1PT_BASE >> 12);
        MMIO64(entry + OFF_FSC) = val64;
    }

    // Write DDTP register
    ddtp = (ROOT_DDT_BASE >> 12) | DDTP_MODE_1LVL;
    MMIO32(IOMMU_DDTP_ADDR)   = (uint32_t)(ddtp & 0xFFFFFFFF);
    MMIO32(IOMMU_DDTP_ADDR+4) = (uint32_t)(ddtp >> 32);
}

// -----------------------------------------------------------------------------
// Page-queue (PQ) init
// -----------------------------------------------------------------------------
void iommu_pq_init(void) {
    uint64_t pqb = ((0x00D00000ULL >> 2) & PQB_PPN_MASK) | PQ_LOG2SZ_1;
    uint32_t cqh;

    // Program PQ base
    MMIO32(IOMMU_PQB_ADDR)   = (uint32_t)(pqb & 0xFFFFFFFF);
    MMIO32(IOMMU_PQB_ADDR+4) = (uint32_t)(pqb >> 32);

    // Tail = head
    cqh = MMIO32(IOMMU_PQH_ADDR);
    MMIO32(IOMMU_PQT_ADDR) = cqh;

    // Enable & poll
    MMIO32(IOMMU_PQCSR_ADDR) = PQCSR_CQEN | PQCSR_CIE;
    do {
        cqh = MMIO32(IOMMU_PQCSR_ADDR);
    } while ((cqh & PQCSR_CQON) == 0);
}

// -----------------------------------------------------------------------------
// Command-queue (CQ) init
// -----------------------------------------------------------------------------
void iommu_cq_init(void) {
    uint64_t cqb = ((0x00EF0000ULL >> 2) & CQB_PPN_MASK) | CQ_LOG2SZ_1;
    uint32_t cqh;

    // Program CQ base
    MMIO32(IOMMU_CQB_ADDR)   = (uint32_t)(cqb & 0xFFFFFFFF);
    MMIO32(IOMMU_CQB_ADDR+4) = (uint32_t)(cqb >> 32);

    // Tail = head
    cqh = MMIO32(IOMMU_CQH_ADDR);
    MMIO32(IOMMU_CQT_ADDR) = cqh;

    // Enable & poll
    MMIO32(IOMMU_CQCSR_ADDR) = CQCSR_CQEN | CQCSR_CIE;
    do {
        cqh = MMIO32(IOMMU_CQCSR_ADDR);
    } while ((cqh & CQCSR_CQON) == 0);
}
