#include <iommu_pts.h>


#if ((MEM_BASE & (SUPERPAGE_SIZE(0)-1)) != 0)
#   error "MEM_BASE must be aligned to level 0 superpage size"
#endif

#if (MEM_BASE >= (SUPERPAGE_SIZE(0)*4))
#   error "MEM_BASE less than four time level 0 superpage size"
#endif

#if (MEM_SIZE < (0x10000000))
#   error "MEM_SIZE not enough (less than 256MB)"
#endif

// Page Table Entries
struct {
    uint64_t stage1;
    uint64_t stage2;
} test_page_perm_table[TEST_PAGE_MAX] = {
    // Index                // S1                       // S2
    [IOMMU_OFF_R]       =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [IOMMU_OFF_W]       =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [IOMMU_BARE_R]      =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [IOMMU_BARE_W]      =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [BARE_TRANS_R1]     =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [BARE_TRANS_R2]     =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [BARE_TRANS_W1]     =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [BARE_TRANS_W2]     =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [S2_ONLY_R]         =   {0,                         PTE_V | PTE_U | PTE_R},    
    [S2_ONLY_W]         =   {0,                         PTE_V | PTE_U | PTE_RW},
    [MSI_R1]            =   {0,                         PTE_V | PTE_U | PTE_R},    
    [TWO_STAGE_R4K]     =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},       
    [TWO_STAGE_R2M]     =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},    
    [TWO_STAGE_R1G]     =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},     
    [MSI_R2]            =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},
    [TWO_STAGE_W4K]     =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RW},
    [IOTINVAL_R1]       =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},
    [IOTINVAL_R2]       =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},
    [WSI_R]             =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_R},
    [WSI_W]             =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RX},
    [MSI_GEN_R]         =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_R},
    [MSI_GEN_W]         =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RX},
    [HPM_R]             =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [HPM_W]             =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [MSI_R3]            =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},
    [MSI_R4]            =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},
    [MSI_R5]            =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},
    [SWITCH1]           =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [SWITCH2]           =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX}, 
    [MSI_W1]            =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [MSI_W2]            =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [MSI_W3]            =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [MSI_W4]            =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [MSI_W5]            =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},
    [PT_TOP]            =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},
};      

// 6x512 PTEs
pte_t s1pt[6][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));


/**
 *  Setup first-stage PTEs
 */
void s1pt_init(){

    uintptr_t addr;

    addr = 0x00000000;

    //# Set 4 leaf 1GiB PTEs.
    // RWX permissions, User bit, Valid flag and A/D bits set
    for(int i = 0; i < 4; i++){
        s1pt[0][i] = 
            STAGE1_PERM_1GIB | (addr >> 2);  
        addr +=  SUPERPAGE_SIZE(0);
    }

    fence_i();

    // Set non-leaf PTE (s1pt[0][3]) pointing to first-lvl PT (s1pt[1][0])
    s1pt[0][3] = 
        PTE_V | ((((uintptr_t)&s1pt[1][0]) >> 2) & PTE_PPN_MSK);

    // addr = MEM_BASE;
    addr = 0xC0000000ULL;

    // Clear first-level table entries
    for(int i = 0; i < 512; i++) s1pt[1][i] = 0;

    fence_i();

    //# Set 64 leaf 2MiB PTEs.
    //  i = [0,63]
    for(int i = 0; i <  MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
        s1pt[1][i] = 
           STAGE1_PERM_2MIB | (addr >> 2);
        addr +=  SUPERPAGE_SIZE(1);
    }

    // Setup non-leaf entry pointing to second-level PT (s1pt[2][0]) from root table
    s1pt[0][4] =
        PTE_V | ((((uintptr_t)&s1pt[2][0]) >> 2) & PTE_PPN_MSK);

    // Setup non-leaf entry pointing to third-level PT (s1pt[3][0]) from second-lvl table
    s1pt[2][0] = 
        PTE_V | ((((uintptr_t)&s1pt[3][0]) >> 2) & PTE_PPN_MSK);

    addr = TEST_VPAGE_BASE;
    // addr = TEST_PPAGE_BASE;

    //# Fill s1pt[3][i] with 4-kiB PTEs
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        if (i >= STRESS_START && i < STRESS_TOP)
        {
            s1pt[3][i] = (addr >> 2) | PTE_AD |
            PTE_V | PTE_U | PTE_RWX;
        }
        else
        {
            s1pt[3][i] = (addr >> 2) | PTE_AD |
                test_page_perm_table[i].stage1;
        }
        addr +=  PAGE_SIZE;
    }

    // Setup non-leaf entry pointing to fourth-level PT (s1pt[4][0]) from second-lvl table
    s1pt[2][1] = 
        PTE_V | ((((uintptr_t)&s1pt[4][0]) >> 2) & PTE_PPN_MSK);

    addr = 4 * SUPERPAGE_SIZE(0) + SUPERPAGE_SIZE(1);   // 4 * 0x4000_0000 + 0x0020_0000 = 1_0020_0000

    //# Fill s1pt[4][i] with 4-kiB PTEs
    for(int i = 0; i < 512; i++){
        if (i >= STRESS_START && i < STRESS_TOP)
        {
            s1pt[4][i] = (addr >> 2) | PTE_AD |
            PTE_V | PTE_U | PTE_RWX;
        }
        else
        {
            s1pt[4][i] = (addr >> 2) | PTE_AD |
                test_page_perm_table[i].stage1;
        }
        addr +=  PAGE_SIZE;
    }

    // Setup non-leaf entry pointing to fifth-level PT (s1pt[5][0]) from root table
    s1pt[0][5] = 
        PTE_V | ((((uintptr_t)&s1pt[5][0]) >> 2) & PTE_PPN_MSK);
    
    addr = 5 * SUPERPAGE_SIZE(0);   // 5 * 0x4000_0000 = 0x1_4000_0000

    //# Fill s1pt[5][i] with leaf 2MiB PTEs.
    for(int i = 0; i < 512; i++){
        s1pt[5][i] = (addr >> 2) |
             PTE_V | PTE_AD | PTE_RWX;  
        addr +=  SUPERPAGE_SIZE(1);
    }
}

// Root table (Sv39x4) (2048 PTEs pointing to 16-kiB pages)
pte_t s2pt_root[PAGE_SIZE*4/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE*4)));

// n-level tables (5x512 PTEs pointing to 4-kiB pages)
pte_t s2pt[5][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));

/**
 *  Setup second-stage PTEs 
 */
void s2pt_init(){

    // Clear root-table entries
    for(int i = 0; i < 2048; i++){
        s2pt_root[i] = 0;
    }

    uintptr_t addr = 0x0;
    
    //# Set 4 leaf 1GiB PTEs.
    for(int i = 0; i < 4; i++){
        s2pt_root[i] = 
            STAGE2_PERM_1GIB | (addr >> 2);  
        addr +=  SUPERPAGE_SIZE(0);
    }

    fence_i();

    //# Set non-leaf entry pointing to the base address of s2pt
    s2pt_root[3] =
        PTE_V | ((((uintptr_t)&s2pt[0][0]) >> 2) & PTE_PPN_MSK);     // s2pt_root[3]

    addr = MEM_BASE;

    // Clear first-level table entries
    for(int i = 0; i < 512; i++) s2pt[0][i] = 0;

    fence_i();

    //# Set 64 leaf 2MiB PTEs.
    // i = [0,63]
    for(int i = 0; i < MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
        s2pt[0][i] = 
            STAGE2_PERM_2MIB | (addr >> 2);
        addr +=  SUPERPAGE_SIZE(1);
    }    

    //# Non-leaf entries
    // Set 2 non-leaf entries pointing to the base address of the second-level table (s2pt[1][0])
    s2pt_root[4] =
        PTE_V | ((((uintptr_t)&s2pt[1][0]) >> 2) & PTE_PPN_MSK);

    s2pt_root[2047] =
        PTE_V | ((((uintptr_t)&s2pt[1][0]) >> 2) & PTE_PPN_MSK);

    // Set first and last entries of s2pt[1][i] pointing to the base address of the third-level table
    s2pt[1][0] = 
        PTE_V | ((((uintptr_t)&s2pt[2][0]) >> 2) & PTE_PPN_MSK);

    s2pt[1][511] = 
        PTE_V | (((((uintptr_t)&s2pt[2][0]) >> 2)) & PTE_PPN_MSK);

    addr = TEST_PPAGE_BASE;

    //# Fill the third-level table (s2pt[2][i]) with 4-kiB PTEs
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        if (i >= STRESS_START && i < STRESS_TOP)
        {
            s2pt[2][i] = (addr >> 2) | PTE_AD |
            PTE_V | PTE_U | PTE_RWX;
        }
        else
        {
            s2pt[2][i] = (addr >> 2) | PTE_AD |
                test_page_perm_table[i].stage2;
        }
        addr +=  PAGE_SIZE;
    }

    // Non-leaf entry pointing to s2pt[3][0]
    s2pt[1][1] = 
        PTE_V | ((((uintptr_t)&s2pt[3][0]) >> 2) & PTE_PPN_MSK);

    addr = TEST_PPAGE_BASE;

    //# Fill s2pt[3][i] with 4-kiB PTEs defined in the table
    for(int i = 0; i < 512; i++){
        if (i >= STRESS_START && i < STRESS_TOP)
        {
            s2pt[3][i] = (addr >> 2) | PTE_AD |
            PTE_V | PTE_U | PTE_RWX;
        }
        else
        {
            s2pt[3][i] = (addr >> 2) | PTE_AD |
                test_page_perm_table[i].stage2;
        }
        addr +=  PAGE_SIZE;
    }

    // Non-leaf entry pointing to s2pt[4][0] from root table
    s2pt_root[5] =
        PTE_V | ((((uintptr_t)&s2pt[4][0]) >> 2) & PTE_PPN_MSK);

    addr = TEST_PPAGE_BASE;

    //# Fill s2pt[4][i] with 2-MiB PTEs
    for(int i = 0; i < 512; i++){
        s2pt[4][i] = (addr >> 2) |
             PTE_V | PTE_U | PTE_AD | PTE_RWX;  
        addr +=  SUPERPAGE_SIZE(1);
    }
}

// Swap two 4-kiB PTEs associated with the permission table (first-stage w/ V=1)
void s1pt_switch(){
    pte_t temp = s1pt[3][SWITCH1];
    s1pt[3][SWITCH1] = s1pt[3][SWITCH2];
    s1pt[3][SWITCH2] = temp;
}

// Swap two 4-kiB PTEs associated with the permission table (second-stage)
void s2pt_switch(){
    pte_t temp = s2pt[2][SWITCH1];
    s2pt[2][SWITCH1] = s2pt[2][SWITCH2];
    s2pt[2][SWITCH2] = temp;
}
