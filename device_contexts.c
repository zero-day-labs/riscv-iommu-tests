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
// uint64_t ddt[3][DDT_N_ENTRIES * 8] __attribute__((aligned(PAGE_SIZE)));

void ddt_init()
{
    // Init all entries to zero
    for (int i = 0; i < (DDT_N_ENTRIES * 8); i++)
    {
        root_ddt[i] = 0;
        // ddt[0][i] = 0;
        // ddt[1][i] = 0;
        // ddt[2][i] = 0;
    }

    // // Configure non-leaf DDT entries
    // ddt[0][280] = DC_TC_VALID | (((uintptr_t)&ddt[1][0]) >> 2);
    // ddt[1][450] = DC_TC_VALID | (((uintptr_t)&ddt[2][0]) >> 2);

    // // Configure 4 DCs in the DDT (3LVL mode)
    // for (int i = 0; i < 32; i=i+8)
    // {
    //     ddt[2][i+0] = test_dc_tc_table[BASIC];                          // DC.tc
    //     ddt[2][i+1] = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE);  // DC.iohgatp
    //     ddt[2][i+1] |= (0x0ABCULL << GSCID_OFF);
    //     ddt[2][i+2] = (0x0DEFULL << PSCID_OFF);                         // DC.ta
    //     ddt[2][i+3] = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE);        // DC.fsc
    //     ddt[2][i+4] = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE);      // DC.msiptp
    //     ddt[2][i+5] = MSI_ADDR_MASK;                                    // DC.msi_addr_mask
    //     ddt[2][i+6] = MSI_ADDR_PATTERN;                                 // DC.msi_addr_pattern
    //     ddt[2][i+7] = 0;                                                // DC.reserved
    // }

    // Configure 4 DCs in the root DDT (1LVL mode)
    for (int i = 0; i < 32; i=i+8)
    {
        root_ddt[i+0] = test_dc_tc_table[BASIC];                                // DC.tc
        root_ddt[i+1] = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_BARE); // DC.iohgatp
        root_ddt[i+1] |= (0x0ABCULL << GSCID_OFF);
        root_ddt[i+2] = (0x0DEFULL << PSCID_OFF);                               // DC.ta
        root_ddt[i+3] = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_BARE);         // DC.fsc
        root_ddt[i+4] = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_OFF);       // DC.msiptp
        root_ddt[i+5] = MSI_ADDR_MASK;                                          // DC.msi_addr_mask
        root_ddt[i+6] = MSI_ADDR_PATTERN;                                       // DC.msi_addr_pattern
        root_ddt[i+7] = 0;                                                      // DC.reserved
    }
}

void set_iommu_off()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uint64_t ddtp_addr = IOMMU_REG_ADDR(IOMMU_DDTP_OFFSET);
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_OFF);

    write64(ddtp_addr, ddtp);
}

void set_iommu_bare()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uint64_t ddtp_addr = IOMMU_REG_ADDR(IOMMU_DDTP_OFFSET);
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_BARE);

    write64(ddtp_addr, ddtp);
}

void set_iommu_1lvl()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uint64_t ddtp_addr = IOMMU_REG_ADDR(IOMMU_DDTP_OFFSET);
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_1LVL);
    
    write64(ddtp_addr, ddtp);
}

void set_iosatp_bare()
{
    for (int i = 0; i < 32; i=i+8)
    {
        root_ddt[i+2] = (0x0DEFULL << PSCID_OFF);                         // DC.ta
        root_ddt[i+3] = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_BARE);   // DC.fsc
    }
}

void set_iosatp_sv39()
{
    for (int i = 0; i < 32; i=i+8)
    {
        root_ddt[i+2] = (0x0DEFULL << PSCID_OFF);                         // DC.ta
        root_ddt[i+3] = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_SV39);   // DC.fsc
    }
}

void set_iohgatp_bare()
{
    for (int i = 0; i < 32; i=i+8)
    {
        root_ddt[i+1] = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_BARE);  // DC.iohgatp
        root_ddt[i+1] |= (0x0ABCULL << GSCID_OFF);
    }
}

void set_iohgatp_sv39x4()
{
    for (int i = 0; i < 32; i=i+8)
    {
        root_ddt[i+1] = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_SV39X4);  // DC.iohgatp
        root_ddt[i+1] |= (0x0ABCULL << GSCID_OFF);
    }
}

void set_msi_off()
{
    for (int i = 0; i < 32; i=i+8)
    {
        root_ddt[i+4] = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_OFF);        // DC.msiptp
        root_ddt[i+5] = MSI_ADDR_MASK;                                          // DC.msi_addr_mask
        root_ddt[i+6] = MSI_ADDR_PATTERN;                                       // DC.msi_addr_pattern
    }
}

void set_msi_flat()
{
    for (int i = 0; i < 32; i=i+8)
    {
        root_ddt[i+4] = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_FLAT);       // DC.msiptp
        root_ddt[i+5] = MSI_ADDR_MASK;                                          // DC.msi_addr_mask
        root_ddt[i+6] = MSI_ADDR_PATTERN;                                       // DC.msi_addr_pattern
    }
}
