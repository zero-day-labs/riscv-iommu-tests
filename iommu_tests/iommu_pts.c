#include <iommu_tests/iommu_tests.h>


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
    // Index            // S1                        // S2
    [S1RWX_S2RWX]       =   {PTE_V | PTE_RWX,           PTE_V | PTE_RWX},     
    [S1RWX_S2RW]        =   {PTE_V | PTE_RWX,           PTE_V | PTE_RW},     
    [S1RWX_S2RX]        =   {PTE_V | PTE_RWX,           PTE_V | PTE_RX},     
    [S1RWX_S2R]         =   {PTE_V | PTE_RWX,           PTE_V | PTE_R},
    [S1RWX_S2X]         =   {PTE_V | PTE_RWX,           PTE_V | PTE_X},     
    [S1RW_S2RWX]        =   {PTE_V | PTE_RW,            PTE_V | PTE_RWX},     
    [S1RW_S2RW]         =   {PTE_V | PTE_RW,            PTE_V | PTE_RW},      
    [S1RW_S2RX]         =   {PTE_V | PTE_RW,            PTE_V | PTE_RX},  
    [S1RW_S2R]          =   {PTE_V | PTE_RW,            PTE_V | PTE_R},    
    [S1RW_S2X]          =   {PTE_V | PTE_RW,            PTE_V | PTE_X},       
    [S1RX_S2RWX]        =   {PTE_V | PTE_RX,            PTE_V | PTE_RWX},    
    [S1RX_S2RW]         =   {PTE_V | PTE_RX,            PTE_V | PTE_RW},     
    [S1RX_S2RX]         =   {PTE_V | PTE_RX,            PTE_V | PTE_RX},
    [S1RX_S2R]          =   {PTE_V | PTE_RX,            PTE_V | PTE_R},    
    [S1RX_S2X]          =   {PTE_V | PTE_RX,            PTE_V | PTE_X}, 
    [S1R_S2RWX]         =   {PTE_V | PTE_R,             PTE_V | PTE_RWX},   
    [S1R_S2RW]          =   {PTE_V | PTE_R,             PTE_V | PTE_RW},    
    [S1R_S2RX]          =   {PTE_V | PTE_R,             PTE_V | PTE_RX},
    [S1R_S2R]           =   {PTE_V | PTE_R,             PTE_V | PTE_R},    
    [S1R_S2X]           =   {PTE_V | PTE_R,             PTE_V | PTE_X},      
    [S1X_S2RWX]         =   {PTE_V | PTE_X,             PTE_V | PTE_RWX},   
    [S1X_S2RW]          =   {PTE_V | PTE_X,             PTE_V | PTE_RW},    
    [S1X_S2RX]          =   {PTE_V | PTE_X,             PTE_V | PTE_RX}, 
    [S1X_S2R]           =   {PTE_V | PTE_X,             PTE_V | PTE_R},   
    [S1X_S2X]           =   {PTE_V | PTE_X,             PTE_V | PTE_X},  
    [S1RWX_S2URWX]      =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},            
    [S1RWX_S2URW]       =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RW},             
    [S1RWX_S2URX]       =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RX}, 
    [S1RWX_S2UR]        =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_R},            
    [S1RWX_S2UX]        =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_X},          
    [S1RW_S2URWX]       =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_RWX},            
    [S1RW_S2URW]        =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_RW},         
    [S1RW_S2URX]        =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_RX},
    [S1RW_S2UR]         =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_R},         
    [S1RW_S2UX]         =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_X},          
    [S1RX_S2URWX]       =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_RWX},            
    [S1RX_S2URW]        =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_RW},         
    [S1RX_S2URX]        =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_RX}, 
    [S1RX_S2UR]         =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_R},        
    [S1RX_S2UX]         =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_X},    
    [S1R_S2URWX]        =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_RWX},        
    [S1R_S2URW]         =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_RW},         
    [S1R_S2URX]         =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_RX},
    [S1R_S2UR]          =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_R},         
    [S1R_S2UX]          =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_X},      
    [S1X_S2URWX]        =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_RWX},        
    [S1X_S2URW]         =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_RW},         
    [S1X_S2URX]         =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_RX},   
    [S1X_S2UR]          =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_R},      
    [S1X_S2UX]          =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_X},          
    [S1URWX_S2RWX]      =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_RWX},            
    [S1URWX_S2RW]       =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_RW},         
    [S1URWX_S2RX]       =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_RX},
    [S1URWX_S2R]        =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_R},         
    [S1URWX_S2X]        =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_X},          
    [S1URW_S2RWX]       =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_RWX},        
    [S1URW_S2RW]        =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_RW},         
    [S1URW_S2RX]        =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_RX},
    [S1URW_S2R]         =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_R},         
    [S1URW_S2X]         =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_X},          
    [S1URX_S2RWX]       =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_RWX},        
    [S1URX_S2RW]        =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_RW},         
    [S1URX_S2RX]        =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_RX},
    [S1URX_S2R]         =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_R},         
    [S1URX_S2X]         =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_X},    
    [S1UR_S2RWX]        =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_RWX},        
    [S1UR_S2RW]         =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_RW},         
    [S1UR_S2RX]         =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_RX}, 
    [S1UR_S2R]          =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_R},        
    [S1UR_S2X]          =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_X},         
    [S1UX_S2RWX]        =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_RWX},        
    [S1UX_S2RW]         =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_RW},         
    [S1UX_S2RX]         =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_RX}, 
    [S1UX_S2R]          =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_R},        
    [S1UX_S2X]          =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_X},          
    [S1URWX_S2URWX]     =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},            
    [S1URWX_S2URW]      =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RW},             
    [S1URWX_S2URX]      =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RX},
    [S1URWX_S2UR]       =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_R},              
    [S1URWX_S2UX]       =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_X},          
    [S1URW_S2URWX]      =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RWX},            
    [S1URW_S2URW]       =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RW},         
    [S1URW_S2URX]       =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RX}, 
    [S1URW_S2UR]        =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_R},        
    [S1URW_S2UX]        =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_X},          
    [S1URX_S2URWX]      =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_RWX},        
    [S1URX_S2URW]       =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_RW},         
    [S1URX_S2URX]       =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_RX},
    [S1URX_S2UR]        =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_R},         
    [S1URX_S2UX]        =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_X},   
    [S1UR_S2URWX]       =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_RWX},
    [S1UR_S2URW]        =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_RW},
    [S1UR_S2URX]        =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_RX},
    [S1UR_S2UR]         =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},
    [S1UR_S2UX]         =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_X},       
    [S1UX_S2URWX]       =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_RWX},        
    [S1UX_S2URW]        =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_RW},         
    [S1UX_S2URX]        =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_RX},
    [S1UX_S2UR]         =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_R},         
    [S1UX_S2UX]         =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_X},
    [S1I_S2I]           =   {0,                         0}, 
    [S1RWX_S2I]         =   {PTE_V | PTE_RWX,           0}, 
    [S1RW_S2I]          =   {PTE_V | PTE_RW,            0}, 
    [S1I_S2URWX]        =   {0,                         PTE_V | PTE_U | PTE_RWX},  
    [S1I_S2UX]          =   {0,                         PTE_V | PTE_U | PTE_X},
    [S1I_S2UR]          =   {0,                         PTE_V | PTE_U | PTE_R},
    [S1I_S2URW]         =   {0,                         PTE_V | PTE_U | PTE_RW},
    [SCRATCHPAD]        =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},           
    [SWITCH1]           =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},      
    [SWITCH2]           =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},
    [IDMA_WRDEST]       =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},
    [S1RWX_S2URWX_MSI]  =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},
    [TOP]               =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},
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
            PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
        addr +=  SUPERPAGE_SIZE(0);
    }

    // Set non-leaf PTE (s1pt[0][1]) pointing to first-lvl PT (s1pt[1][0])
    s1pt[0][MEM_BASE/SUPERPAGE_SIZE(0)] = 
        PTE_V | (((uintptr_t)&s1pt[1][0]) >> 2);

    addr = MEM_BASE;

    // Clear first-level table entries
    for(int i = 0; i < 512; i++) s1pt[1][i] = 0;

    //# Set 64 leaf 2MiB PTEs.
    //  i = [0,63]
    for(int i = 0; i <  MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
        s1pt[1][i] = 
           PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
        addr +=  SUPERPAGE_SIZE(1);
    }

    // Setup non-leaf entry pointing to second-level PT (s1pt[2][0]) from root table
    s1pt[0][4] =
        PTE_V | (((uintptr_t)&s1pt[2][0]) >> 2);

    // s1pt[0][5] =
    //     PTE_V | PTE_U | PTE_AD | (((uintptr_t)&s1pt[2][0]) >> 2);

    // Setup non-leaf entry pointing to third-level PT (s1pt[3][0]) from second-lvl table
    s1pt[2][0] = 
        PTE_V | (((uintptr_t)&s1pt[3][0]) >> 2);

    addr = TEST_VPAGE_BASE;

    //# Fill s1pt[3][i] with 4-kiB PTEs
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        s1pt[3][i] = (addr >> 2) | PTE_AD |
            test_page_perm_table[i].stage1;
        addr +=  PAGE_SIZE;
    }

    // Setup non-leaf entry pointing to fourth-level PT (s1pt[4][0]) from second-lvl table
    s1pt[2][1] = 
        PTE_V | (((uintptr_t)&s1pt[4][0]) >> 2);

    addr = 4 * SUPERPAGE_SIZE(0) + SUPERPAGE_SIZE(1);   // 4 * 0x4000_0000 + 0x0020_0000 = 1_0020_0000

    //# Fill s1pt[4][i] with 4-kiB PTEs
    for(int i = 0; i < 512; i++){
        s1pt[4][i] = (addr >> 2) | 
            PTE_V | PTE_AD | PTE_RWX; 
        addr +=  PAGE_SIZE;
    }  

    // Setup non-leaf entry pointing to fifth-level PT (s1pt[5][0]) from root table
    s1pt[0][5] = 
        PTE_V | (((uintptr_t)&s1pt[5][0]) >> 2);
    
    addr = 5 * SUPERPAGE_SIZE(0);   // 5 * 0x4000_0000 = 0x1_4000_0000

    //# Fill s1pt[5][i] with leaf 2MiB PTEs.
    for(int i = 0; i < 512; i++){
        s1pt[5][i] = (addr >> 2) |
             PTE_V | PTE_AD | PTE_RWX;  
        addr +=  SUPERPAGE_SIZE(1);
    }

    // // Configure satp according to the originating privilege mode
    // uintptr_t iosatp = (((uintptr_t)s1pt) >> 12) | (0x8ULL << 60);

    // // If VS-mode (Guest OS running on top of a hypervisor)
    // if(curr_priv == PRIV_VS){
    //     CSRW(satp, satp);
    // }
    
    // // If HS-mode (Hypervisor/Host OS) or M-mode
    // else if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
    //     CSRW(CSR_VSATP, satp);
    // }
    
    // // Insufficient privilege level
    // else {
    //     ERROR("");
    // }
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
    // RWX permissions, User bit, Valid flag and A/D bits set
    // REMARK: For second-stage address translation, all memory accesses are considered to be U-lvl accesses
    for(int i = 0; i < 4; i++){
        s2pt_root[i] = 
            PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);  
        addr +=  SUPERPAGE_SIZE(0);
    }

    //# Set non-leaf entry pointing to the base address of s2pt
    s2pt_root[MEM_BASE/SUPERPAGE_SIZE(0)] =
        PTE_V | (((uintptr_t)&s2pt[0][0]) >> 2);     // s2pt_root[2]

    addr = MEM_BASE;

    // Clear first-level table entries
    for(int i = 0; i < 512; i++) s2pt[0][i] = 0;

    //# Set 64 leaf 2MiB PTEs.
    // i = [0,63]
    for(int i = 0; i < MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
        s2pt[0][i] = 
            PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);
        addr +=  SUPERPAGE_SIZE(1);
    }    

    //# Non-leaf entries
    // Set 2 non-leaf entries pointing to the base address of the second-level table (s2pt[1][0])
    s2pt_root[4] =
        PTE_V | (((uintptr_t)&s2pt[1][0]) >> 2);

    s2pt_root[2047] =
        PTE_V | (((uintptr_t)&s2pt[1][0]) >> 2);

    // Set first and last entries of s2pt[1][i] pointing to the base address of the third-level table
    s2pt[1][0] = 
        PTE_V | (((uintptr_t)&s2pt[2][0]) >> 2);

    s2pt[1][511] = 
        PTE_V | (((uintptr_t)&s2pt[2][0]) >> 2);

    addr = TEST_PPAGE_BASE;

    //# Fill the third-level table (s2pt[2][i]) with 4-kiB PTEs
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        s2pt[2][i] = (addr >> 2) | PTE_AD |
            test_page_perm_table[i].stage2;  
        addr +=  PAGE_SIZE;
    }

    // Non-leaf entry pointing to s2pt[3][0]
    s2pt[1][1] = 
        PTE_V | (((uintptr_t)&s2pt[3][0]) >> 2);

    addr = TEST_PPAGE_BASE;

    //# Fill s2pt[3][i] with 4-kiB PTEs defined in the table
    for(int i = 0; i < 512; i++){
        s2pt[3][i] = (addr >> 2) | 
            PTE_V | PTE_U | PTE_AD | PTE_RWX; 
        addr +=  PAGE_SIZE;
    }  

    // Non-leaf entry pointing to s2pt[4][0] from root table
    s2pt_root[5] =
        PTE_V | (((uintptr_t)&s2pt[4][0]) >> 2);

    addr = TEST_PPAGE_BASE;

    //# Fill s2pt[4][i] with 2-MiB PTEs
    for(int i = 0; i < 512; i++){
        s2pt[4][i] = (addr >> 2) |
             PTE_V | PTE_U | PTE_AD | PTE_RWX;  
        addr +=  SUPERPAGE_SIZE(1);
    }

    // // Configure hgatp with the PPN of the root table + Sv39x4 scheme
    // if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
    //     uintptr_t hgatp = (((uintptr_t)s2pt_root) >> 12) | (0x8ULL << 60);
    //     CSRW(CSR_HGATP, hgatp);
    // } else {
    //     ERROR("trying to set hs hgatp from lower privilege");
    // }
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
