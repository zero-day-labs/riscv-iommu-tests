/**
 * !NOTES:
 * 
 *  -   For now, we have only one DMA device (iDMA) to test the IOMMU. As each DC is associated with a different MSI
 *      PT, mask and pattern, it would be necessary to allocate a buffer for each DC, with the N of MSI PTEs associated
 *      with the corresponding MSI mask
 * 
 *  -   To test MSI address translation, virt_page_base(S1RWX_S2URWX_MSI) must be called to get the virtual address (0x103 << 12)
 *      that indexes the first-stage 4-kiB PTE that maps to the GPPN (0x103), which fulfills the condition to be considered
 *      as the GPA of a virtual IF (vIMSIC). The iDMA must be then programmed to write to this Virtual Address. The MSI
 *      address translation process should be triggered since:
 *                 MASK               GPPN                 MASK             PATTERN
 *          (~0010_1001_0110) & (0001_0000_0011) == (~0010_1001_0110) & (0011_1000_0101)
 * 
 *  -   To translate the MSI address, an interrupt file number is extracted using the MSI mask and the input GPPN
 *          extract(GPPN, msi_addr_mask) = extract(0001_0000_0011, 0010_1001_0110) = 00001 = IFN
 * 
 * -    The flat MSI PT is indexed using this IF number as follows: MSI PTE address = (msiptp.ppn << 12) | (IFN << 4)
 */

#include <iommu_tests/msi_pts.h>

// 32 MSI PTEs, each PTE is 16-bytes, base address aligned to 4-kiB
// When having more than 1 device, it must be upgraded to a 2-dimensional array
uint64_t msi_pt[MSI_N_ENTRIES * 2] __attribute__((aligned(PAGE_SIZE)));

void msi_pt_init()
{
    uintptr_t addr = MSI_BASE_IF_SPA;

    // Init all entries to zero
    for (int i = 0; i < MSI_N_ENTRIES; i++)
    {
        msi_pt[i] = 0;
    }

    // Fill MSI PT with entries.
    for(int i = 0; i < MSI_N_ENTRIES; i++){
        msi_pt[i] = (addr >> 2) | MSI_PTE_VALID | MSI_PTE_BT_MODE;
        addr +=  PAGE_SIZE;
    }

    // DC.msiptp is programed with the base address of msi_pt[] in device_contexts.c
}