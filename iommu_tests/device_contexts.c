/**
 * !NOTES:
 * 
 *  -   
 */

#include <iommu_tests/device_contexts.h>

#if (DEVICE_ID_WIDTH > (24))
#   error "Error: device_id can not be wider than 24 bits"
#endif

uint64_t test_dc_tc_table[TEST_PAGE_MAX] = {

    [BASIC]         =   DC_TC_VALID,
    [DISABLE_TF]    =   DC_TC_VALID | DC_TC_DTF,
    [TOP]           =   DC_TC_VALID  
};     

// First and second-stage page tables (Already configured)
extern pte_t s1pt[][512];
extern pte_t s2pt_root[];

// MSI page tables (Configured in msi_pts.c)
extern uint64_t msi_pt[];

// 64 entries of 8 DWs each
uint64_t root_ddt[DDT_N_ENTRIES * 8] __attribute__((aligned(PAGE_SIZE)));

void ddt_init()
{
    uint64_t ddtp_addr = IOMMU_REG_ADDR(IOMMU_DDTP_OFFSET);

    // Init all entries to zero
    for (int i = 0; i < DDT_N_ENTRIES; i++)
    {
        root_ddt[i] = 0;
    }

    // iDMA device used to test has AXI_ID = 0
    root_ddt[0] = test_dc_tc_table[BASIC];                                // DC.tc
    root_ddt[1] = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_SV39X4); // DC.iohgatp
    root_ddt[2] = 0;                                                      // DC.ta
    root_ddt[3] = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_SV39);         // DC.fsc
    root_ddt[4] = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_FLAT);       // DC.msiptp
    root_ddt[5] = MSI_ADDR_MASK;                                          // DC.msi_addr_mask
    root_ddt[6] = MSI_ADDR_PATTERN;                                       // DC.msi_addr_pattern
    root_ddt[7] = 0;                                                      // DC.reserved

    // Program ddtp register with DDT mode and root DDT base PPN
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDT_MODE);
    write64(ddtp_addr, ddtp);
    
}
