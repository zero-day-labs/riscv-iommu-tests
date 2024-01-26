#ifndef MSI_PAGE_TABLES_H
#define MSI_PAGE_TABLES_H

#include <iommu_tests.h>

// Number of entries of the MSI Page Table (PoT, associated with N of set bits in the MSI mask)
#define MSI_N_ENTRIES       (32)

// SPA of the base Interrupt File, i.e., physical address of the page (IF) pointed to by the first entry of the MSI PT.
// 0x8000_0000 + 0x0400_0000 = 0x84000000
#define MSI_BASE_IF_SPA (MEM_BASE+(MEM_SIZE/4))

// MSI PTE fields
#define MSI_PTE_VALID       (1ULL << 0 )    // Valid
#define MSI_PTE_BT_MODE     (3ULL << 1 )    // MSI Mode (3 for basic-translate mode)
#define MSI_PTE_MRIF_MODE   (1ULL << 1 )    // MSI Mode (1 for MRIF)
#define MSI_PTE_CUSTOM      (1ULL << 63)    // Custom format (Not used)

#define MSI_PTE_MRIF_ADDR_MASK  (0x3FFFFFFFFFFF80ULL)
#define MSI_PTE_NID9_0_MASK     (0x3FFULL)
#define MSI_PTE_NPPN_MASK       (0x3FFFFFFFFFFC00ULL)
#define MSI_PTE_NID10_MASK      (1ULL << 60)

#define NOTICE_ADDR_1           (0x83008000ULL)
#define NOTICE_ADDR_2           (0x83009000ULL)
#define NOTICE_DATA             (0x7ACUL)

#define INT_ID_1                (300UL)
#define INT_OFFSET_1            ((INT_ID_1 >> 2) & (~0xFUL))
#define INT_IP_IDX_1            (INT_OFFSET_1 / 8)
#define INT_IE_IDX_1            (INT_IP_IDX_1 + 1)
#define INT_MASK_1              (1ULL << (INT_ID_1 & 0x3FUL))

#define INT_ID_2                (600UL)
#define INT_OFFSET_2            ((INT_ID_2 >> 2) & (~0xFUL))
#define INT_IP_IDX_2            (INT_OFFSET_2 / 8)
#define INT_IE_IDX_2            (INT_IP_IDX_2 + 1)
#define INT_MASK_2              (1ULL << (INT_ID_2 & 0x3FUL))

void msi_pt_init(void);
void mrif_init(void);

#endif  /* MSI_PAGE_TABLES_H */