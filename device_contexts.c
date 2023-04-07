/**
 * !NOTES:
 * 
 *  -   
 */

#include <device_contexts.h>

#if (DEVICE_ID_WIDTH > (24))
#   error "Error: device_id can not be wider than 24 bits"
#endif

uint64_t test_dc_tc_table[TEST_DC_MAX] = {

    [TC_RSVD_SET]   =   DC_TC_VALID | DC_TC_RSV,
    [EN_PRI]        =   DC_TC_VALID | DC_TC_EN_PRI,
    [T2GPA]         =   DC_TC_VALID | DC_TC_T2GPA,
    [SADE]          =   DC_TC_VALID | DC_TC_SADE,
    [SBE]           =   DC_TC_VALID | DC_TC_SBE,
    [SXL]           =   DC_TC_VALID | DC_TC_SXL,
    [INVALID]       =   0,
    [BASIC]         =   DC_TC_VALID,
    [DISABLE_TF]    =   DC_TC_VALID | DC_TC_DTF,
    [DC_TOP]        =   DC_TC_VALID
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
    for (int i = 0; i < DDT_N_ENTRIES * 8; i++)
    {
        root_ddt[i] = 0;
    }

    for (int i = 0; i < 32; i=i+8)
    {
        root_ddt[i+0] = test_dc_tc_table[BASIC];                          // DC.tc
        root_ddt[i+1] = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE);  // DC.iohgatp
        root_ddt[i+2] = 0;                                                // DC.ta
        root_ddt[i+3] = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE);        // DC.fsc
        root_ddt[i+4] = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE);      // DC.msiptp
        root_ddt[i+5] = MSI_ADDR_MASK;                                    // DC.msi_addr_mask
        root_ddt[i+6] = MSI_ADDR_PATTERN;                                 // DC.msi_addr_pattern
        root_ddt[i+7] = 0;                                                // DC.reserved
    }

    // Program ddtp register with DDT mode and root DDT base PPN
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDT_MODE);
    write64(ddtp_addr, ddtp);
}
