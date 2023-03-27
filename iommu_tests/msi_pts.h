#ifndef MSI_PAGE_TABLES_H
#define MSI_PAGE_TABLES_H

#include <iommu_tests/iommu_tests.h>

// Number of entries of the MSI Page Table (POT, associated with N of set bits in the MSI mask)
#define MSI_N_ENTRIES       (32)

// SPA of the base Interrupt File, i.e., physical address of the page (IF) pointed to by the first entry of the MSI PT.
// 0x8000_0000 + 0x0400_0000 = 0x84000000
#define MSI_BASE_IF_SPA (MEM_BASE+(MEM_SIZE/4))

// DC.tc flags
#define MSI_PTE_VALID       (1ULL << 0 )    // Valid
#define MSI_PTE_BT_MODE     (3ULL << 1 )    // MSI Mode (3 for basic-translate mode)
#define MSI_PTE_MRIF_MODE   (1ULL << 1 )    // MSI Mode (1 for MRIF)
#define MSI_PTE_CUSTOM      (1ULL << 63)    // Custom format (Not used)

void msi_pt_init(void);

#endif  /* MSI_PAGE_TABLES_H */