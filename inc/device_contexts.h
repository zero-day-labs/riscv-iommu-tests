#ifndef DEVICE_CONTEXTS_H
#define DEVICE_CONTEXTS_H

#include <iommu_tests.h>

// Number of entries of the root DDT (4-kiB / 64 bytes p/ entry)
#define DDT_N_ENTRIES       (0x1000 / 64)   // 64 entries

// HW device_id of the DMA device. Defined for invalidation
#define DEVICE_ID           (1ULL)

// Mask for ddtp.PPN (ddtp[53:10])
#define DDTP_PPN_MASK    (0x3FFFFFFFFFFC00ULL)

// iosatp encoding to configure DC.fsc
#define IOSATP_MODE_BARE    (0x0ULL << 60)
#define IOSATP_MODE_SV39    (0x8ULL << 60)
// iohgatp encoding to configure DC.iohgatp
#define IOHGATP_MODE_BARE   (0x0ULL << 60)
#define IOHGATP_MODE_SV39X4 (0x8ULL << 60)

// MSI translation mode encoding to configure DC.msiptp
#define MSIPTP_MODE_OFF     (0x0ULL << 60)
#define MSIPTP_MODE_FLAT    (0x1ULL << 60)

// MSI address mask and pattern
#define MSI_ADDR_MASK       (0x100286ULL)   // 0001_0000_0000_0010_1000_0110
#define MSI_ADDR_PATTERN    (0x000385ULL)   // 0000_0000_0000_0011_1000_0101
// MSI GPA                  0x100103ULL        0001_0000_0000_0001_0000_0011 (IFN 10001)
// MSI GPA                  0x100183ULL        0001_0000_0000_0001_1000_0011 (IFN 10101)

// Define DDT mode
#define DDTP_MODE_OFF   (0ULL)
#define DDTP_MODE_BARE  (1ULL)
#define DDTP_MODE_1LVL  (2ULL)
#define DDTP_MODE_2LVL  (3ULL)
#define DDTP_MODE_3LVL  (4ULL)

// DC.tc flags
#define DC_TC_VALID     (1ULL << 0 )    // Valid
#define DC_TC_EN_ATS    (1ULL << 1 )    // EN PCIe Address Translation Services
#define DC_TC_EN_PRI    (1ULL << 2 )    // EN PCIe Page Request If
#define DC_TC_T2GPA     (1ULL << 3 )    // The IOMMU respond ATS Requests with GPAs
#define DC_TC_DTF       (1ULL << 4 )    // Disable report of translation faults through the FQ
#define DC_TC_PDTV      (1ULL << 5 )    // PC associated to this DC
#define DC_TC_PRPR      (1ULL << 6 )    // Include PASID in IOMMU-generated PRGR message if associated PR has a PASID
#define DC_TC_GADE      (1ULL << 7 )    // The IOMMU updates second-stage A and D bits atomically
#define DC_TC_SADE      (1ULL << 8 )    // The IOMMU updates first-stage A and D bits atomically
#define DC_TC_DPE       (1ULL << 9 )    // Default process_id value is 0 for requests without valid process_id
#define DC_TC_SBE       (1ULL << 10)    // Endianness for implicit accesses to PDT and first-stage PTEs
#define DC_TC_SXL       (1ULL << 11)    // To define first-stage translation scheme
#define DC_TC_RSV       (1ULL << 12)    // To raise fault for setting rsvd fields

#define GSCID_OFF       (44)
#define PSCID_OFF       (12)

// Device Context Indexes
enum test_dc {
    INVALID,
    TC_RSVD_SET,
    EN_PRI,
    T2GPA,
    SADE,
    SBE,
    SXL,
    BASIC,
    DISABLE_TF,
    DC_TOP = (DDT_N_ENTRIES-1),
    TEST_DC_MAX
};

void ddt_init(void);
void set_iommu_off(void);
void set_iommu_bare(void);
void set_iommu_1lvl(void);
void set_iosatp_sv39(void);
void set_iohgatp_sv39x4(void);
void set_iosatp_bare(void);
void set_iohgatp_bare(void);
void set_msi_flat(void);

#endif  /* DEVICE_CONTEXTS_H */