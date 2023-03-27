#include <stdint.h>
#include <stdio.h>
#include <page_tables.h> 
#include <csrs.h>


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
    uint64_t vs;
    uint64_t h;
} test_page_perm_table[TEST_PAGE_MAX] = {
    // Index            // VS                       // G
    [VSRWX_GRWX]    =   {PTE_V | PTE_RWX,           PTE_V | PTE_RWX},     
    [VSRWX_GRW]     =   {PTE_V | PTE_RWX,           PTE_V | PTE_RW},     
    [VSRWX_GRX]     =   {PTE_V | PTE_RWX,           PTE_V | PTE_RX},     
    [VSRWX_GR]      =   {PTE_V | PTE_RWX,           PTE_V | PTE_R},
    [VSRWX_GX]      =   {PTE_V | PTE_RWX,           PTE_V | PTE_X},     
    [VSRW_GRWX]     =   {PTE_V | PTE_RW,            PTE_V | PTE_RWX},     
    [VSRW_GRW]      =   {PTE_V | PTE_RW,            PTE_V | PTE_RW},      
    [VSRW_GRX]      =   {PTE_V | PTE_RW,            PTE_V | PTE_RX},  
    [VSRW_GR]       =   {PTE_V | PTE_RW,            PTE_V | PTE_R},    
    [VSRW_GX]       =   {PTE_V | PTE_RW,            PTE_V | PTE_X},       
    [VSRX_GRWX]     =   {PTE_V | PTE_RX,            PTE_V | PTE_RWX},    
    [VSRX_GRW]      =   {PTE_V | PTE_RX,            PTE_V | PTE_RW},     
    [VSRX_GRX]      =   {PTE_V | PTE_RX,            PTE_V | PTE_RX},
    [VSRX_GR]       =   {PTE_V | PTE_RX,            PTE_V | PTE_R},    
    [VSRX_GX]       =   {PTE_V | PTE_RX,            PTE_V | PTE_X}, 
    [VSR_GRWX]      =   {PTE_V | PTE_R,             PTE_V | PTE_RWX},   
    [VSR_GRW]       =   {PTE_V | PTE_R,             PTE_V | PTE_RW},    
    [VSR_GRX]       =   {PTE_V | PTE_R,             PTE_V | PTE_RX},
    [VSR_GR]        =   {PTE_V | PTE_R,             PTE_V | PTE_R},    
    [VSR_GX]        =   {PTE_V | PTE_R,             PTE_V | PTE_X},      
    [VSX_GRWX]      =   {PTE_V | PTE_X,             PTE_V | PTE_RWX},   
    [VSX_GRW]       =   {PTE_V | PTE_X,             PTE_V | PTE_RW},    
    [VSX_GRX]       =   {PTE_V | PTE_X,             PTE_V | PTE_RX}, 
    [VSX_GR]        =   {PTE_V | PTE_X,             PTE_V | PTE_R},   
    [VSX_GX]        =   {PTE_V | PTE_X,             PTE_V | PTE_X},  
    [VSRWX_GURWX]   =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},            
    [VSRWX_GURW]    =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RW},             
    [VSRWX_GURX]    =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RX}, 
    [VSRWX_GUR]     =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_R},            
    [VSRWX_GUX]     =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_X},          
    [VSRW_GURWX]    =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_RWX},            
    [VSRW_GURW]     =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_RW},         
    [VSRW_GURX]     =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_RX},
    [VSRW_GUR]      =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_R},         
    [VSRW_GUX]      =   {PTE_V | PTE_RW,            PTE_V | PTE_U | PTE_X},          
    [VSRX_GURWX]    =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_RWX},            
    [VSRX_GURW]     =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_RW},         
    [VSRX_GURX]     =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_RX}, 
    [VSRX_GUR]      =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_R},        
    [VSRX_GUX]      =   {PTE_V | PTE_RX,            PTE_V | PTE_U | PTE_X},    
    [VSR_GURWX]     =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_RWX},        
    [VSR_GURW]      =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_RW},         
    [VSR_GURX]      =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_RX},
    [VSR_GUR]       =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_R},         
    [VSR_GUX]       =   {PTE_V | PTE_R,             PTE_V | PTE_U | PTE_X},      
    [VSX_GURWX]     =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_RWX},        
    [VSX_GURW]      =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_RW},         
    [VSX_GURX]      =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_RX},   
    [VSX_GUR]       =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_R},      
    [VSX_GUX]       =   {PTE_V | PTE_X,             PTE_V | PTE_U | PTE_X},          
    [VSURWX_GRWX]   =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_RWX},            
    [VSURWX_GRW]    =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_RW},         
    [VSURWX_GRX]    =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_RX},
    [VSURWX_GR]     =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_R},         
    [VSURWX_GX]     =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_X},          
    [VSURW_GRWX]    =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_RWX},        
    [VSURW_GRW]     =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_RW},         
    [VSURW_GRX]     =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_RX},
    [VSURW_GR]      =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_R},         
    [VSURW_GX]      =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_X},          
    [VSURX_GRWX]    =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_RWX},        
    [VSURX_GRW]     =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_RW},         
    [VSURX_GRX]     =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_RX},
    [VSURX_GR]      =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_R},         
    [VSURX_GX]      =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_X},    
    [VSUR_GRWX]     =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_RWX},        
    [VSUR_GRW]      =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_RW},         
    [VSUR_GRX]      =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_RX}, 
    [VSUR_GR]       =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_R},        
    [VSUR_GX]       =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_X},         
    [VSUX_GRWX]     =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_RWX},        
    [VSUX_GRW]      =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_RW},         
    [VSUX_GRX]      =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_RX}, 
    [VSUX_GR]       =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_R},        
    [VSUX_GX]       =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_X},          
    [VSURWX_GURWX]  =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RWX},            
    [VSURWX_GURW]   =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RW},             
    [VSURWX_GURX]   =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_RX},
    [VSURWX_GUR]    =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_R},              
    [VSURWX_GUX]    =   {PTE_V | PTE_U | PTE_RWX,   PTE_V | PTE_U | PTE_X},          
    [VSURW_GURWX]   =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RWX},            
    [VSURW_GURW]    =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RW},         
    [VSURW_GURX]    =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_RX}, 
    [VSURW_GUR]     =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_R},        
    [VSURW_GUX]     =   {PTE_V | PTE_U | PTE_RW,    PTE_V | PTE_U | PTE_X},          
    [VSURX_GURWX]   =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_RWX},        
    [VSURX_GURW]    =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_RW},         
    [VSURX_GURX]    =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_RX},
    [VSURX_GUR]     =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_R},         
    [VSURX_GUX]     =   {PTE_V | PTE_U | PTE_RX,    PTE_V | PTE_U | PTE_X},   
    [VSUR_GURWX]    =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_RWX},
    [VSUR_GURW]     =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_RW},
    [VSUR_GURX]     =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_RX},
    [VSUR_GUR]      =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_R},
    [VSUR_GUX]      =   {PTE_V | PTE_U | PTE_R,     PTE_V | PTE_U | PTE_X},       
    [VSUX_GURWX]    =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_RWX},        
    [VSUX_GURW]     =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_RW},         
    [VSUX_GURX]     =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_RX},
    [VSUX_GUR]      =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_R},         
    [VSUX_GUX]      =   {PTE_V | PTE_U | PTE_X,     PTE_V | PTE_U | PTE_X},
    [VSI_GI]        =   {0,                         0}, 
    [VSRWX_GI]      =   {PTE_V | PTE_RWX,           0}, 
    [VSRW_GI]       =   {PTE_V | PTE_RW,            0}, 
    [VSI_GURWX]     =   {0,                         PTE_V | PTE_U | PTE_RWX},  
    [VSI_GUX]       =   {0,                         PTE_V | PTE_U | PTE_X},
    [VSI_GUR]       =   {0,                         PTE_V | PTE_U | PTE_R},
    [VSI_GURW]      =   {0,                         PTE_V | PTE_U | PTE_RW},
    [SCRATCHPAD]    =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},           
    [SWITCH1]       =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},      
    [SWITCH2]       =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},      
    [TOP]           =   {PTE_V | PTE_RWX,           PTE_V | PTE_U | PTE_RWX},     
};      

// Three vectors of PTEs. Each vector contains the max number of PTEs we can arrange within a page (512 PTEs)
// Each vector represents a level of the page table (three for Sv39x4)
// hspt[0] leaf entries are 1 GiB PTEs
// hspt[1] leaf entries are 2 MiB PTEs
// hspt[2] leaf entries are 4 kiB PTEs
pte_t hspt[3][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));

/**
 *  Setup satp and first-stage PTEs 
 */
void hspt_init(){

    uintptr_t addr;

    // for(int j = 0; j < 3; j++)
    //     for(int i = 0; i < 512; i++)
    //         hspt[j][i] = 0;

    // addr(0) = 0x 0000_0000;  addr'(0) = 0x 0000_0000;  PPN[29:28] = 00;
    // addr(1) = 0x 4000_0000;  addr'(1) = 0x 1000_0000;  PPN[29:28] = 01;
    // addr(2) = 0x 8000_0000;  addr'(2) = 0x 2000_0000;  PPN[29:28] = 10;
    // addr(3) = 0x C000_0000;  addr'(3) = 0x 3000_0000;  PPN[29:28] = 11;

    //# Set 4 leaf 1GiB PTEs.
    // RWX permissions, valid flag and A/D bits set
    // Also, bits [29:28] (PPN[2]) are set according to the comments before
    addr = 0x00000000;
    for(int i = 0; i < 4; i++){
        hspt[0][i] = 
            PTE_V | PTE_AD | PTE_RWX | (addr >> 2);
        addr +=  SUPERPAGE_SIZE(0);     // 1 GiB
    }

    //# Setup non-leaf entries.
    // hspt[0][4] points to the base addr of the second-level table represented by hspt[1][0].
    // hspt[1][0] points to the base addr of the third-level table represented by hspt[2][0].
    hspt[0][4] =
        PTE_V | (((uintptr_t)&hspt[1][0]) >> 2);
    hspt[1][0] = 
        PTE_V | (((uintptr_t)&hspt[2][0]) >> 2);

    //# Fill third-level (4-kiB) table with PTEs. Use permissions table
    // All PTEs have Access and Dirty bits set
    addr = TEST_PPAGE_BASE;
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hspt[2][i] = (addr >> 2) | PTE_AD |
            test_page_perm_table[i].vs;  
        addr += PAGE_SIZE;
    }

    // hspt[0][MEM_BASE/SUPERPAGE_SIZE(0)] = 
    //     PTE_V | (((uintptr_t)&hspt[1][0]) >> 2);

    // addr = MEM_BASE;
    // for(int i = 0; i < 127; i++){
    //     hspt[1][i] = 
    //         PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
    //     addr +=  SUPERPAGE_SIZE(1);
    // }
    // hspt[1][127] = 
    //     PTE_V | (((uintptr_t)&hspt[2]) >> 2);     

    // addr = 0x80000000 + (127*SUPERPAGE_SIZE(1));
    // for(int i = 0; i < 512; i++){
    //     hspt[2][i] = 
    //         PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
    //     addr +=  PAGE_SIZE;
    // }

    //# Setup satp register.
    // satp can only be configured by HS-mode (hypervisor) and M-mode software.
    // If V=1, vsatp substitutes for the usual satp.
    // so instructions that normally read or modify satp actually access vsatp instead.
    // satp.MODE is configured as Sv39. The address of hspt is the root address of the PT
    if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
        uintptr_t satp = (((uintptr_t)hspt) >> 12) | (0x8ULL << 60);
        CSRW(satp, satp);
    } else {
        ERROR("trying to set hs level satp from lower privilege");
    }
}

// 6x512 PTEs
pte_t vspt[6][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));


/**
 *  Setup vsatp and first-stage PTEs 
 */
void vspt_init(){

    uintptr_t addr;

    addr = 0x00000000;

    //# Set 4 leaf 1GiB PTEs.
    // RWX permissions, User bit, Valid flag and A/D bits set
    for(int i = 0; i < 4; i++){
        vspt[0][i] = 
            PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
        addr +=  SUPERPAGE_SIZE(0);
    }

    // Set non-leaf PTE (vspt[0][1]) pointing to first-lvl PT (vspt[1][0])
    vspt[0][MEM_BASE/SUPERPAGE_SIZE(0)] = 
        PTE_V | (((uintptr_t)&vspt[1][0]) >> 2);

    addr = MEM_BASE;

    // Clear first-level table entries
    for(int i = 0; i < 512; i++) vspt[1][i] = 0;

    //# Set 255 leaf 2MiB PTEs.
    //  i = [0,255]
    for(int i = 0; i <  MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
        vspt[1][i] = 
           PTE_V | PTE_AD | PTE_RWX | (addr >> 2);  
        addr +=  SUPERPAGE_SIZE(1);
    }

    // Setup non-leaf entry pointing to second-level PT (vspt[2][0]) from root table
    vspt[0][4] =
        PTE_V | (((uintptr_t)&vspt[2][0]) >> 2);

    // vspt[0][5] =
    //     PTE_V | PTE_U | PTE_AD | (((uintptr_t)&vspt[2][0]) >> 2);

    // Setup non-leaf entry pointing to third-level PT (vspt[3][0]) from second-lvl table
    vspt[2][0] = 
        PTE_V | (((uintptr_t)&vspt[3][0]) >> 2);

    addr = TEST_VPAGE_BASE;

    //# Fill vspt[3][i] with 4-kiB PTEs
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        vspt[3][i] = (addr >> 2) | PTE_AD |
            test_page_perm_table[i].vs;
        addr +=  PAGE_SIZE;
    }

    // Setup non-leaf entry pointing to fourth-level PT (vspt[4][0]) from second-lvl table
    vspt[2][1] = 
        PTE_V | (((uintptr_t)&vspt[4][0]) >> 2);

    addr = 4 * SUPERPAGE_SIZE(0) + SUPERPAGE_SIZE(1);   // 4 * 0x4000_0000 + 0x0020_0000 = 1_0020_0000

    //# Fill vspt[4][i] with 4-kiB PTEs
    for(int i = 0; i < 512; i++){
        vspt[4][i] = (addr >> 2) | 
            PTE_V | PTE_AD | PTE_RWX; 
        addr +=  PAGE_SIZE;
    }  

    // Setup non-leaf entry pointing to fifth-level PT (vspt[5][0]) from root table
    vspt[0][5] = 
        PTE_V | (((uintptr_t)&vspt[5][0]) >> 2);
    
    addr = 5 * SUPERPAGE_SIZE(0);   // 5 * 0x4000_0000 = 0x1_4000_0000

    //# Fill vspt[5][i] with leaf 2MiB PTEs.
    for(int i = 0; i < 512; i++){
        vspt[5][i] = (addr >> 2) |
             PTE_V | PTE_AD | PTE_RWX;  
        addr +=  SUPERPAGE_SIZE(1);
    }  

    //# Configure vsatp or satp according to the originating privilege mode
    uintptr_t satp = (((uintptr_t)vspt) >> 12) | (0x8ULL << 60);

    // If VS-mode (Guest OS running on top of a hypervisor)
    if(curr_priv == PRIV_VS){
        CSRW(satp, satp);
    }
    
    // If HS-mode (Hypervisor/Host OS) or M-mode
    else if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
        CSRW(CSR_VSATP, satp);
    }
    
    // Insufficient privilege level
    else {
        ERROR("");
    }
}

// Root table (Sv39x4) (2048 PTEs pointing to 16-kiB pages)
pte_t hpt_root[PAGE_SIZE*4/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE*4)));

// n-level tables (5x512 PTEs pointing to 4-kiB pages)
pte_t hpt[5][PAGE_SIZE/sizeof(pte_t)] __attribute__((aligned(PAGE_SIZE)));

/**
 *  Setup hgatp and second-stage PTEs 
 */
void hpt_init(){

    // Clear root-table entries
    for(int i = 0; i < 2048; i++){
        hpt_root[i] = 0;
    }

    uintptr_t addr = 0x0;
    
    //# Set 4 leaf 1GiB PTEs.
    // RWX permissions, User bit, Valid flag and A/D bits set
    // REMARK: For second-stage address translation, all memory accesses are considered to be U-lvl accesses
    for(int i = 0; i < 4; i++){
        hpt_root[i] = 
            PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);  
        addr +=  SUPERPAGE_SIZE(0);
    }

    //# Set non-leaf entry pointing to the base address of hpt
    hpt_root[MEM_BASE/SUPERPAGE_SIZE(0)] =
        PTE_V | (((uintptr_t)&hpt[0][0]) >> 2);     // hpt_root[1]

    addr = MEM_BASE;

    // Clear first-level table entries
    for(int i = 0; i < 512; i++) hpt[0][i] = 0;

    //# Set 255 leaf 2MiB PTEs.
    // i = [0,255]
    for(int i = 0; i < MEM_SIZE/SUPERPAGE_SIZE(1)/2; i++){
        hpt[0][i] = 
            PTE_V | PTE_U | PTE_AD | PTE_RWX | (addr >> 2);
        addr +=  SUPERPAGE_SIZE(1);
    }    

    //# Non-leaf entries
    // Set 2 non-leaf entries pointing to the base address of the second-level table (hpt[1][0])
    hpt_root[4] =
        PTE_V | (((uintptr_t)&hpt[1][0]) >> 2);

    hpt_root[2047] =
        PTE_V | (((uintptr_t)&hpt[1][0]) >> 2);

    // Set first and last entries of hpt[1][i] pointing to the base address of the third-level table
    hpt[1][0] = 
        PTE_V | (((uintptr_t)&hpt[2][0]) >> 2);

    hpt[1][511] = 
        PTE_V | (((uintptr_t)&hpt[2][0]) >> 2);

    addr = TEST_PPAGE_BASE;

    //# Fill the third-level table (hpt[2][i]) with 4-kiB PTEs
    for(int i = 0; i < TEST_PAGE_MAX; i++){
        hpt[2][i] = (addr >> 2) | PTE_AD |
            test_page_perm_table[i].h;  
        addr +=  PAGE_SIZE;
    }

    // Non-leaf entry pointing to hpt[3][0]
    hpt[1][1] = 
        PTE_V | (((uintptr_t)&hpt[3][0]) >> 2);

    addr = TEST_PPAGE_BASE;

    //# Fill hpt[3][i] with 4-kiB PTEs defined in the table
    for(int i = 0; i < 512; i++){
        hpt[3][i] = (addr >> 2) | 
            PTE_V | PTE_U | PTE_AD | PTE_RWX; 
        addr +=  PAGE_SIZE;
    }  

    // Non-leaf entry pointing to hpt[4][0] from root table
    hpt_root[5] =
        PTE_V | (((uintptr_t)&hpt[4][0]) >> 2);

    addr = TEST_PPAGE_BASE;

    //# Fill hpt[4][i] with 2-MiB PTEs
    for(int i = 0; i < 512; i++){
        hpt[4][i] = (addr >> 2) |
             PTE_V | PTE_U | PTE_AD | PTE_RWX;  
        addr +=  SUPERPAGE_SIZE(1);
    }

    //# Configure hgatp with the PPN of the root table + Sv39x4 scheme
    if(curr_priv == PRIV_HS || curr_priv == PRIV_M){
        uintptr_t hgatp = (((uintptr_t)hpt_root) >> 12) | (0x8ULL << 60);
        CSRW(CSR_HGATP, hgatp);
    } else {
        ERROR("trying to set hs hgatp from lower privilege");
    }
}

// Swap two 4-kiB PTEs associated with the permission table (first-stage w/ V=0)
void hspt_switch(){
    pte_t temp = hspt[2][SWITCH1];
    hspt[2][SWITCH1] = hspt[2][SWITCH2];
    hspt[2][SWITCH2] = temp;
}

// Swap two 4-kiB PTEs associated with the permission table (first-stage w/ V=1)
void vspt_switch(){
    pte_t temp = vspt[3][SWITCH1];
    vspt[3][SWITCH1] = vspt[3][SWITCH2];
    vspt[3][SWITCH2] = temp;
}

// Swap two 4-kiB PTEs associated with the permission table (second-stage)
void hpt_switch(){
    pte_t temp = hpt[2][SWITCH1];
    hpt[2][SWITCH1] = hpt[2][SWITCH2];
    hpt[2][SWITCH2] = temp;
}
