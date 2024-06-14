/**
 * !NOTES:
 * 
 *  -   For now, we have only one DMA device (iDMA) to test the IOMMU. As each DC is associated with a different MSI
 *      PT, mask and pattern, it would be necessary to allocate a buffer for each DC, with the N of MSI PTEs associated
 *      with the corresponding MSI mask
 * 
 *  -   To test MSI address translation, virt_page_base(S1URWX_S2URWX_MSI) must be called to get the virtual address (0x103 << 12)
 *      that indexes the first-stage 4-kiB PTE that maps to the GPPN (0x103), which fulfills the condition to be considered
 *      as the GPA of a virtual IF (vIMSIC). The iDMA must be then programmed to write to this Virtual Address. The MSI
 *      address translation process should be triggered since:
 *                 MASK               GPPN                 MASK             PATTERN
 *          (~0010_1000_0110) & (0001_1000_0011) == (~0010_1000_0110) & (0011_1000_0101)
 *           (1101_0111_1001) & (0001_1000_0011) ==  (1101_0111_1001) & (0011_1000_0101)
 *                              (0001_0000_0001) ==  (0001_0000_0001)
 * 
 *  -   To translate the MSI address, an interrupt file number is extracted using the MSI mask and the input GPPN
 *          extract(GPPN, msi_addr_mask) = extract(0001_0000_0000_0001_1000_0011, 0001_0000_0000_0010_1000_0110) = 10101 = IFN
 * 
 * -    The flat MSI PT is indexed using this IF number as follows: MSI PTE address = (msiptp.ppn << 12) | (IFN << 4)
 */

#include <msi_pts.h>

/**
 *                 MASK               GPPN                 MASK             PATTERN
 *          (~0010_1000_0110) & (0001_1000_0000) == (~0010_1000_0110) & (0011_1000_0110)
 *           (1101_0111_1001) & (0001_1000_0000) ==  (1101_0111_1001) & (0011_1000_0110)
 *                              (0001_0000_0000) ==  (0001_0000_0000)
 * 
 * extract(0001_0000_0000_0001_1000_0000,   addr
 *         0001_0000_0000_0010_1000_0110)   mask
 *            1             0  1     00
 */

// MSI GPA 1                0x100102ULL        0001_0000_0000_0001_0000_0010 (IFN 10001 - 17)
// MSI GPA 2                0x100106ULL        0001_0000_0000_0001_0000_0110 (IFN 10011 - 19)
// MSI GPA 3                0x100180ULL        0001_0000_0000_0001_1000_0000 (IFN 10100 - 20)
// MSI GPA 4                0x100182ULL        0001_0000_0000_0001_1000_0010 (IFN 10101 - 21)
// MSI GPA 5                0x100186ULL        0001_0000_0000_0001_1000_0110 (IFN 10111 - 23)

// 32 MSI PTEs, each PTE is 16-bytes, base address aligned to 4-kiB
uint64_t msi_pt[MSI_N_ENTRIES * 2] __attribute__((aligned(PAGE_SIZE)));

// MRIF
uint64_t mrif[64] __attribute__((aligned(512)));

void msi_pt_init()
{
    uintptr_t addr = MSI_BASE_IF_SPA;

    // Init all entries to zero
    for (int i = 0; i < (MSI_N_ENTRIES * 2); i++)
    {
        msi_pt[i] = 0;
    }

    // Fill MSI PT with entries in BT mode.
    for(int i = 0; i < (MSI_N_ENTRIES * 2); i+=2)
    {
        msi_pt[i]   = MSI_PTE_VALID | MSI_PTE_BT_MODE | (addr >> 2);
        msi_pt[i+1] = 0;
        addr +=  PAGE_SIZE;
    }

    fence_i();

    // Configure MSI PTEs in MRIF mode
    // MSI GPA 3                0x100180ULL        0001_0000_0000_0001_1000_0000 (IFN 10100 - 20)
    // Validate two-stage MSI translation
    msi_pt[40]   = MSI_PTE_VALID | MSI_PTE_MRIF_MODE | ((((uintptr_t)mrif) >> 2) & MSI_PTE_MRIF_ADDR_MASK);
    msi_pt[40+1] = (NOTICE_DATA & MSI_PTE_NID9_0_MASK) | ((((uintptr_t)NOTICE_ADDR_1) >> 2) & MSI_PTE_NPPN_MASK) | 
                    ((NOTICE_DATA << 50) & MSI_PTE_NID10_MASK);

    // MSI GPA 4                0x100182ULL        0001_0000_0000_0001_1000_0010 (IFN 10101 - 21)
    // Validate second-stage-only MSI translation
    msi_pt[42]   = MSI_PTE_VALID | MSI_PTE_MRIF_MODE | ((((uintptr_t)mrif) >> 2) & MSI_PTE_MRIF_ADDR_MASK);
    msi_pt[42+1] = (NOTICE_DATA & MSI_PTE_NID9_0_MASK) | ((((uintptr_t)NOTICE_ADDR_2) >> 2) & MSI_PTE_NPPN_MASK) | 
                    ((NOTICE_DATA << 50) & MSI_PTE_NID10_MASK);

    // MSI GPA 5                0x100186ULL        0001_0000_0000_0001_1000_0110 (IFN 10111 - 23)
    // Validate error propagation
    msi_pt[46]   = MSI_PTE_VALID | MSI_PTE_MRIF_MODE | ((((uintptr_t)mrif) >> 2) & MSI_PTE_MRIF_ADDR_MASK) | MSI_PTE_CUSTOM;
    msi_pt[46+1] = ((uint64_t)NOTICE_DATA & MSI_PTE_NID9_0_MASK) | ((((uintptr_t)NOTICE_ADDR_1) >> 2) & MSI_PTE_NPPN_MASK) | 
                    (((uint64_t)NOTICE_DATA << 50) & MSI_PTE_NID10_MASK);

    // DC.msiptp is programed with the base address of msi_pt[] in device_contexts.c
}

/**
 *  Configure IP and IE bits within the MRIF
 */
void mrif_init()
{
    // Interrupt Identity 1
    // IP
    mrif[INT_IP_IDX_1] = 0;
    // IE
    mrif[INT_IE_IDX_1] = INT_MASK_1;

    // Interrupt Identity 2
    // IP
    mrif[INT_IP_IDX_2] = 0;
    // IE
    // mrif[INT_IE_IDX_2] = INT_MASK_2;
    mrif[INT_IE_IDX_2] = 0;
}