#ifndef PAGE_TABLES_H
#define PAGE_TABLES_H

#include <iommu_tests.h>

#define PT_SIZE (PAGE_SIZE)
#define PAGE_ADDR_MSK (~(PAGE_SIZE - 1))  // ... 1111 1111 0000 0000 0000
#define PAGE_SHIFT (12)

// N = 0:   1 GiB superpages (addr[55:30] = '0) (0x40000000)
// N = 1:   2 MiB superpages (addr[55:21] = '0) (0x00200000)
// N = 2:   4 kiB pages      (addr[55:12] = '0) (0x00001000)
#define SUPERPAGE_SIZE(N) ((PAGE_SIZE) << (((2-N))*9))

#define PT_LVLS (3)  // assumes sv39 for rv64
#define PTE_INDEX_SHIFT(LEVEL) ((9 * (PT_LVLS - 1 - (LEVEL))) + 12)
#define PTE_ADDR_MSK BIT_MASK(12, 44)

#define PTE_INDEX(LEVEL, ADDR) (((ADDR) >> PTE_INDEX_SHIFT(LEVEL)) & (0x1FF))
#define PTE_FLAGS_MSK BIT_MASK(0, 8)

#define PTE_VALID (1ULL << 0)
#define PTE_READ (1ULL << 1)
#define PTE_WRITE (1ULL << 2)
#define PTE_EXECUTE (1ULL << 3)
#define PTE_USER (1ULL << 4)
#define PTE_GLOBAL (1ULL << 5)
#define PTE_ACCESS (1ULL << 6)
#define PTE_DIRTY (1ULL << 7)

#define PTE_V PTE_VALID
#define PTE_AD (PTE_ACCESS | PTE_DIRTY)
#define PTE_U PTE_USER
#define PTE_R (PTE_READ)
#define PTE_RW (PTE_READ | PTE_WRITE)
#define PTE_X (PTE_EXECUTE)
#define PTE_RX (PTE_READ | PTE_EXECUTE)
#define PTE_RWX (PTE_READ | PTE_WRITE | PTE_EXECUTE)

#define PTE_PPN_MSK (0x3FFFFFFFFFFC00ULL)

#define PTE_RSW_OFF 8
#define PTE_RSW_LEN 2
#define PTE_RSW_MSK BIT_MASK(PTE_RSW_OFF, PTE_RSW_LEN)

#define TEST_VADDR_1GIB     (0x80000000ULL)    // vpn[2] = 2
#define TEST_GPADDR_1GIB    (0x80000000ULL)
#define TEST_PADDR_1GIB     (0x80000000ULL)

#define TEST_VADDR_2MIB     (0xC4000000ULL)    // vpn[2]  = 3, vpn[1]  = 32 (0x20)
#define TEST_GPADDR_2MIB    (0xC4000000ULL)    // gppn[2] = 2, gppn[1] = 32 (0x20)
#define TEST_PADDR_2MIB     (0x84000000ULL)    // ppn[2]  = 2, ppn[1]  = 32 (0x20)

#define STAGE1_PERM_1GIB    (PTE_V | PTE_AD | PTE_U | PTE_RWX)
#define STAGE1_PERM_2MIB    (PTE_V | PTE_AD | PTE_U | PTE_RWX)
#define STAGE2_PERM_1GIB    (PTE_V | PTE_AD | PTE_U | PTE_RWX)
#define STAGE2_PERM_2MIB    (PTE_V | PTE_AD | PTE_U | PTE_RWX)

/* ------------------------------------------------------------- */

// Base Supervisor Physical Address of 4-kiB pages
#define TEST_PPAGE_BASE (MEM_BASE+(MEM_SIZE/2))     // 0x8000_0000 + 0x0800_0000 = 0x88000000

// Base Guest Virtual Address of 4-kiB pages:
// Independently of the base address in satp
// PPN[2] selects the fourth entry of the first-stage root table
// PPN[1] selects the first entry of the first-stage second-lvl table.
// This entry points to the table with 4-kiB entries filled according to the permission table
// | PPN[2] | PPN[1] | PPN[0] | OFF |
// |  100b  |   '0   |   '0   |  '0 |
#define TEST_VPAGE_BASE (0x100000000)

enum test_page { 
    IOMMU_OFF_R,
    IOMMU_OFF_W,
    IOMMU_BARE_R,
    IOMMU_BARE_W,
    BARE_TRANS_R1,
    BARE_TRANS_R2,
    BARE_TRANS_W1,
    BARE_TRANS_W2,
    S2_ONLY_R,
    S2_ONLY_W,
    MSI_R1,
    TWO_STAGE_R4K,
    TWO_STAGE_R2M,
    TWO_STAGE_R1G,
    MSI_R2,
    TWO_STAGE_W4K,
    IOTINVAL_R1,
    IOTINVAL_R2,
    WSI_R,
    WSI_W,
    MSI_GEN_R,
    MSI_GEN_W,
    HPM_R,
    HPM_W,
    MSI_R3,
    MSI_R4,
    MSI_R5,
    SWITCH1,
    SWITCH2,
    STRESS_START,
    STRESS_TOP = STRESS_START + N_MAPPINGS,
    MSI_W1 = 258,
    MSI_W2 = 262,
    MSI_W3 = 384,
    MSI_W4 = 386,
    MSI_W5 = 390,
    PT_TOP = 511,
    TEST_PAGE_MAX
};

extern pte_t s1pt[][512];
extern pte_t s2pt_root[];
extern pte_t s2pt[][512];

// Returns the base address of the virtual page specified by 'tp'
static inline uintptr_t virt_page_base(enum test_page tp){
    if(tp < TEST_PAGE_MAX){
        return (uintptr_t)(TEST_VPAGE_BASE+(tp*PAGE_SIZE));
    } else {
        ERROR("trying to get invalid test virtual page address");
    }
}

static inline uintptr_t s1_page_base_limit(enum test_page tp){
    if(tp < TEST_PAGE_MAX){
        return (uintptr_t)((0x20000000000-0x200000)+(tp*PAGE_SIZE));
    } else {
        ERROR("trying to get invalid test page address");
    }
}

// Returns the base address of the physical page specified by 'tp'
static inline uintptr_t phys_page_base(enum test_page tp){
    if(tp < TEST_PAGE_MAX){
        return (uintptr_t)(TEST_PPAGE_BASE+(tp*PAGE_SIZE));
    } else {
        ERROR("trying to get invalid test physical page address");
    }
}

void s1pt_init();
void s2pt_init();
void s1pt_switch();
void s2pt_switch();

#endif /* PAGE_TABLES_H */
