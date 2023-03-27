#ifndef PAGE_TABLES_H
#define PAGE_TABLES_H

#include <iommu_tests/iommu_tests.h>

#define PAGE_SIZE 0x1000ULL     // 4kiB
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

#define PTE_RSW_OFF 8
#define PTE_RSW_LEN 2
#define PTE_RSW_MSK BIT_MASK(PTE_RSW_OFF, PTE_RSW_LEN)

#define PTE_TABLE (0)
#define PTE_PAGE (PTE_RWX)
#define PTE_SUPERPAGE (PTE_PAGE)

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
    S1RWX_S2URWX,
    S1RWX_S2URW,
    S1RWX_S2URX,
    S1RWX_S2UR,
    S1RWX_S2UX,
    S1RW_S2URWX,
    S1RW_S2URW,
    S1RW_S2URX,
    S1RW_S2UR,
    S1RW_S2UX,
    S1RX_S2URWX,
    S1RX_S2URW,
    S1RX_S2URX,
    S1RX_S2UR,
    S1RX_S2UX,
    S1R_S2URWX,
    S1R_S2URW,
    S1R_S2URX,
    S1R_S2UR,
    S1R_S2UX,
    S1X_S2URWX,
    S1X_S2URW,
    S1X_S2URX,
    S1X_S2UR,
    S1X_S2UX,
    S1URWX_S2RWX,
    S1URWX_S2RW,
    S1URWX_S2RX,
    S1URWX_S2R,
    S1URWX_S2X,
    S1URW_S2RWX,
    S1URW_S2RW,
    S1URW_S2RX,
    S1URW_S2R,
    S1URW_S2X,
    S1URX_S2RWX,
    S1URX_S2RW,
    S1URX_S2RX,
    S1URX_S2R,
    S1URX_S2X,
    S1UR_S2RWX,
    S1UR_S2RW,
    S1UR_S2RX,
    S1UR_S2R,
    S1UR_S2X,
    S1UX_S2RWX,
    S1UX_S2RW,
    S1UX_S2RX,
    S1UX_S2R,
    S1UX_S2X,
    S1URWX_S2URWX,
    S1URWX_S2URW,
    S1URWX_S2URX,
    S1URWX_S2UR,
    S1URWX_S2UX,
    S1URW_S2URWX,
    S1URW_S2URW,
    S1URW_S2URX,
    S1URW_S2UR,
    S1URW_S2UX,
    S1URX_S2URWX,
    S1URX_S2URW,
    S1URX_S2URX,
    S1URX_S2UR,
    S1URX_S2UX,
    S1UR_S2URWX,
    S1UR_S2URW,
    S1UR_S2URX,
    S1UR_S2UR,
    S1UR_S2UX,
    S1UX_S2URWX,
    S1UX_S2URW,
    S1UX_S2URX,
    S1UX_S2UR,
    S1UX_S2UX,
    S1RWX_S2RWX,
    S1RWX_S2RW,
    S1RWX_S2RX,
    S1RWX_S2R,
    S1RWX_S2X,
    S1RW_S2RWX,
    S1RW_S2RW,
    S1RW_S2RX,
    S1RW_S2R,
    S1RW_S2X,
    S1RX_S2RWX,
    S1RX_S2RW,
    S1RX_S2RX,
    S1RX_S2R,
    S1RX_S2X,
    S1R_S2RWX,
    S1R_S2RW,
    S1R_S2RX,
    S1R_S2R,
    S1R_S2X,
    S1X_S2RWX,
    S1X_S2RW,
    S1X_S2RX,
    S1X_S2R,
    S1X_S2X,
    S1I_S2I,
    S1RWX_S2I,
    S1RW_S2I,
    S1I_S2URWX,
    S1I_S2UX,
    S1I_S2UR,
    S1I_S2URW,
    SCRATCHPAD,
    SWITCH1,
    SWITCH2,
    IDMA_WRDEST,
    S1RWX_S2URWX_MSI = 259,
    TOP = 511,
    TEST_PAGE_MAX
};

typedef uint64_t pte_t;

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
