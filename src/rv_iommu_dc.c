#include <rv_iommu_dc.h>

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