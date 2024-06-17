/**
 * !NOTES:
 * 
 *  -   
 */

#include <device_contexts.h>

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

/**
 *  Used to simulate multiple VMs and process address spaces 
 */
// uint64_t GSCID_ARRAY[16] = {
//     0x0ABCULL,  // 0
//     0x0ABDULL,  // 1
//     0x0ABEULL,  // 2
//     0x0ABFULL,  // 3
//     0x0ABCULL,  // 4
//     0x0ABDULL,  // 5
//     0x0ABEULL,  // 6
//     0x0ABFULL,  // 7
//     0x0ABCULL,  // 8
//     0x0ABDULL,  // 9
//     0x0ABEULL,  // 10
//     0x0ABFULL,  // 11
//     0x0ABCULL,  // 12
//     0x0ABDULL,  // 13
//     0x0ABEULL,  // 14
//     0x0ABFULL   // 15
// };

// uint64_t PSCID_ARRAY[16] = {
//     0x0DECULL,  // 0
//     0x0DEDULL,  // 1
//     0x0DEEULL,  // 2
//     0x0DEFULL,  // 3
//     0x0DECULL,  // 4
//     0x0DEDULL,  // 5
//     0x0DEEULL,  // 6
//     0x0DEFULL,  // 7
//     0x0DECULL,  // 8
//     0x0DEDULL,  // 9
//     0x0DEEULL,  // 10
//     0x0DEFULL,  // 11
//     0x0DECULL,  // 12
//     0x0DEDULL,  // 13
//     0x0DEEULL,  // 14
//     0x0DEFULL   // 15
// };

uint64_t GSCID_ARRAY[16] = {
    0x0ABCULL,  // 0
    0x0ABCULL,  // 1
    0x0ABCULL,  // 2
    0x0ABCULL,  // 3
    0x0ABCULL,  // 4
    0x0ABCULL,  // 5
    0x0ABCULL,  // 6
    0x0ABCULL,  // 7
    0x0ABCULL,  // 8
    0x0ABCULL,  // 9
    0x0ABCULL,  // 10
    0x0ABCULL,  // 11
    0x0ABCULL,  // 12
    0x0ABCULL,  // 13
    0x0ABCULL,  // 14
    0x0ABCULL   // 15
};

uint64_t PSCID_ARRAY[16] = {
    0x0DEFULL,  // 0
    0x0DEFULL,  // 1
    0x0DEFULL,  // 2
    0x0DEFULL,  // 3
    0x0DEFULL,  // 4
    0x0DEFULL,  // 5
    0x0DEFULL,  // 6
    0x0DEFULL,  // 7
    0x0DEFULL,  // 8
    0x0DEFULL,  // 9
    0x0DEFULL,  // 10
    0x0DEFULL,  // 11
    0x0DEFULL,  // 12
    0x0DEFULL,  // 13
    0x0DEFULL,  // 14
    0x0DEFULL   // 15
};

// First and second-stage page tables (Already configured)
extern pte_t s1pt[][512];
extern pte_t s2pt_root[];

// MSI page tables (Configured in msi_pts.c)
extern uint64_t msi_pt[];

// DDT
ddt_t root_ddt[DDT_N_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

void ddt_init()
{
    // Init all entries to zero
    for (int i = 0; i < DDT_N_ENTRIES; i++)
    {
        root_ddt[i].tc = 0;
    }

    // Configure DCs in the root DDT (1LVL mode)
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].tc = test_dc_tc_table[BASIC];
        root_ddt[i].iohgatp = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_BARE);
        root_ddt[i].iohgatp |= (GSCID_ARRAY[i] << GSCID_OFF);
        root_ddt[i].ta = (PSCID_ARRAY[i] << PSCID_OFF);
        root_ddt[i].fsc = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_BARE);

        if (MSI_TRANSLATION == 1)
        {
            root_ddt[i].msiptp = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_OFF);
            root_ddt[i].msi_addr_mask = MSI_ADDR_MASK;
            root_ddt[i].msi_addr_pattern = MSI_ADDR_PATTERN;
            root_ddt[i].reserved = 0;
        }
    }
}

void set_iosatp_bare()
{
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].ta = (PSCID_ARRAY[i] << PSCID_OFF);
        root_ddt[i].fsc = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_BARE);
    }
}

void set_iosatp_sv39()
{
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].ta = (PSCID_ARRAY[i] << PSCID_OFF);
        root_ddt[i].fsc = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_SV39);
    }
}

void set_iohgatp_bare()
{
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].iohgatp = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_BARE);
        root_ddt[i].iohgatp |= (GSCID_ARRAY[i] << GSCID_OFF);
    }
}

void set_iohgatp_sv39x4()
{
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].iohgatp = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_SV39X4);
        root_ddt[i].iohgatp |= (GSCID_ARRAY[i] << GSCID_OFF);
    }
}

void set_msi_off()
{
    if (MSI_TRANSLATION == 1)
    {
        for (int i = DID_MIN; i < DID_MAX + 1; i++)
        {
            root_ddt[i].msiptp = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_OFF);
            root_ddt[i].msi_addr_mask = MSI_ADDR_MASK;
            root_ddt[i].msi_addr_pattern = MSI_ADDR_PATTERN;
        }
    }
}

void set_msi_flat()
{
    if (MSI_TRANSLATION == 1)
    {
        for (int i = DID_MIN; i < DID_MAX + 1; i++)
        {
            root_ddt[i].msiptp = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_FLAT);
            root_ddt[i].msi_addr_pattern = MSI_ADDR_MASK;
            root_ddt[i].msi_addr_pattern = MSI_ADDR_PATTERN;
        }
    }
}
