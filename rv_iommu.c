#include <rv_iommu.h>
#include <iommu_tests.h>
#include <command_queue.h>
#include <fault_queue.h>
#include <device_contexts.h>
#include <msi_pts.h>
#include <iommu_pts.h>
#include <hpm.h>

/**
 *  IOMMU Memory-mapped registers 
 */
// fctl
uintptr_t fctl_addr = IOMMU_REG_ADDR(IOMMU_FCTL_OFFSET);

// ddtp
uint64_t ddtp_addr = IOMMU_REG_ADDR(IOMMU_DDTP_OFFSET);

// ipsr
uintptr_t ipsr_addr = IOMMU_REG_ADDR(IOMMU_IPSR_OFFSET);

// icvec
uintptr_t icvec_addr = IOMMU_REG_ADDR(IOMMU_ICVEC_OFFSET);

// HPM
uintptr_t iocountovf_addr = IOMMU_REG_ADDR(IOMMU_IOCOUNTOVF_OFFSET);
uintptr_t iocountihn_addr = IOMMU_REG_ADDR(IOMMU_IOCOUNTINH_OFFSET);
uintptr_t iohpmcycles_addr = IOMMU_REG_ADDR(IOMMU_IOHPMCYCLES_OFFSET);
uintptr_t iohpmctr_addr[8] = {
    IOMMU_REG_ADDR(IOMMU_IOHPMCTR_OFFSET + 0*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMCTR_OFFSET + 1*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMCTR_OFFSET + 2*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMCTR_OFFSET + 3*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMCTR_OFFSET + 4*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMCTR_OFFSET + 5*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMCTR_OFFSET + 6*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMCTR_OFFSET + 7*8)
};
uintptr_t iohpmevt_addr[8] = {
    IOMMU_REG_ADDR(IOMMU_IOHPMEVT_OFFSET + 0*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMEVT_OFFSET + 1*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMEVT_OFFSET + 2*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMEVT_OFFSET + 3*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMEVT_OFFSET + 4*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMEVT_OFFSET + 5*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMEVT_OFFSET + 6*8),
    IOMMU_REG_ADDR(IOMMU_IOHPMEVT_OFFSET + 7*8)
};

// MSI Cfg table
uintptr_t msi_addr_cq_addr       = IOMMU_REG_ADDR(IOMMU_MSI_ADDR_3_OFFSET);
uintptr_t msi_data_cq_addr       = IOMMU_REG_ADDR(IOMMU_MSI_DATA_3_OFFSET);
uintptr_t msi_vec_ctl_cq_addr    = IOMMU_REG_ADDR(IOMMU_MSI_VEC_CTL_3_OFFSET);
uintptr_t msi_addr_fq_addr       = IOMMU_REG_ADDR(IOMMU_MSI_ADDR_2_OFFSET);
uintptr_t msi_data_fq_addr       = IOMMU_REG_ADDR(IOMMU_MSI_DATA_2_OFFSET);
uintptr_t msi_vec_ctl_fq_addr    = IOMMU_REG_ADDR(IOMMU_MSI_VEC_CTL_2_OFFSET);
uintptr_t msi_addr_hpm_addr       = IOMMU_REG_ADDR(IOMMU_MSI_ADDR_1_OFFSET);
uintptr_t msi_data_hpm_addr       = IOMMU_REG_ADDR(IOMMU_MSI_DATA_1_OFFSET);
uintptr_t msi_vec_ctl_hpm_addr    = IOMMU_REG_ADDR(IOMMU_MSI_VEC_CTL_1_OFFSET);

// MSI Cfg table data
uint64_t msi_addr_cq     = 0x83000000ULL;
uint32_t msi_data_cq     = 0x00ABCDEFUL;
uint32_t msi_vec_ctl_cq  = 0x1UL;

uint64_t msi_addr_fq     = 0x83001000ULL;
uint32_t msi_data_fq     = 0xFEDCBA00UL;
uint32_t msi_vec_ctl_fq  = 0x0UL;

uint64_t msi_addr_hpm     = 0x83002000ULL;
uint32_t msi_data_hpm     = 0xDEADBEEFUL;
uint32_t msi_vec_ctl_hpm  = 0x0UL;

// DDT
extern uint64_t root_ddt[];

/**
 *  Configure:
 *      - CQ, FQ, S1 and S2 page tables, MSI page tables
 *      - DDT and ddtp register
 *      - MSI Cfg table
 *      - interrupt vectors for each interrupt source
 *      - Hardware Performance Monitor
 */
void init_iommu()
{
    //# Setup the Command Queue:
    // Allocate a buffer of N (POT) entries (16-bytes each). 
    // This buffer must be alligned to the greater of two values: 4-kiB or N x 16 bytes.
    // Configure cqb with the queue size as log2(N) and the base address of the buffer.
    // Set cqt to zero.
    // Enable the CQ by writing 1 to cqcsr.cqen, poll cqcsr.cqon until it reads 1.
    INFO("Configuring CQ");
    cq_init();
    VERBOSE("CQ: Interrupts enabled");

    //# Setup the Fault Queue:
    // Allocate a buffer of N (POT) entries (32-bytes each).
    // This buffer must be alligned to the greater of two values: 4-kiB or N x 32 bytes.
    // Configure fqb with the queue size as log2(N) and the base address of the buffer.
    // Set fqh to zero.
    // Enable the FQ by writing 1 to fqcsr.fqen, poll fqcsr.fqon until it reads 1.
    INFO("Configuring FQ");
    fq_init();
    VERBOSE("FQ: Interrupts enabled");

    //# Configure Page Tables for both translation stages in memory
    // Allocate various buffers to work as multi-level page tables.
    // Fill these buffers with leaf and non-leaf entries (pages and superpages)

    /**
     * Setup hyp page_tables.
     */
    INFO("Configuring second-stage page tables");
    s2pt_init();         // setup iohgatp and second-stage PTEs

    /**
     * Setup guest page tables.
     */
    INFO("Configuring first-stage page tables");
    s1pt_init();        // setup iosatp and first-stage PTEs

    //# Configure a flat MSI Page Table in memory
    // Assume an MSI address mask (52-bits) of 5 bits set.
    // The index for the MSI PT is extracted using this mask and the GPPN associated with the transaction.
    // Using a mask of 5 bits set results in an MSI PT of 2⁵ = 32 entries.
    // Allocate a page of memory for the MSI PT.
    // In order to test the MSI translation mechanism, we must ensure that bits [abcde]
    // are equal in both GPPN and MSI Pattern
    // Example:     MSI Mask:       ... 0010 0110 1001
    //              MSI Pattern:    ... 00Xa bXXc XdeX
    //              GPPN            ... 00Ya bYYc YdeY
    INFO("Configuring MSI page tables and MRIF");
    msi_pt_init();
    mrif_init();

    //# Program the ddtp
    // Assume device_id width same as AXI ID width. Assume DC extended format.
    // Allocate a page of memory to use as the root table of the DDT (Aligned buffer).
    // To do this, determine the number of entries of the root DDT, based on the size of each entry = 4-kiB / 64 bytes.
    // Initialize all entries to zero.
    // Assume support for 3-LVL DDTs.
    // Program the ddtp register with the LVL as the spec specifies and the base address of the page.

    //# Fill the DDT with DCs
    // We can use a table with a structure similar to that used for defining different configurations for PTEs.
    // Since we are using a single DMA-capable device to issue reads and writes to memory, 
    // we will only have one entry in the DDT indexed with the AXI ID of the device (device_id).
    // Save the base address of the first-stage root table in DC.iosatp. Same for second-stage and DC.iohgatp.
    // Define an MSI address mask of 5 bits set for the DC.
    // Define an MSI address pattern for the DC, following the format defined in the mask.
    INFO("Configuring DDT");
    ddt_init();
    set_iommu_off();

    //# Configure MSI Config Table
    INFO("Configuring MSI config table");
    // CQ
    write64(msi_addr_cq_addr,    msi_addr_cq);
    write32(msi_data_cq_addr,    msi_data_cq);
    write32(msi_vec_ctl_cq_addr, msi_vec_ctl_cq);

    // FQ
    write64(msi_addr_fq_addr,    msi_addr_fq);
    write32(msi_data_fq_addr,    msi_data_fq);
    write32(msi_vec_ctl_fq_addr, msi_vec_ctl_fq);

    // HPM
    write64(msi_addr_hpm_addr,    msi_addr_hpm);
    write32(msi_data_hpm_addr,    msi_data_hpm);
    write32(msi_vec_ctl_hpm_addr, msi_vec_ctl_hpm);

    INFO("Configuring IGS to WSI");
    //# Configure the IOMMU to generate interrupts as WSI by default
    set_ig_wsi();

    // //# Configure the IOMMU to generate interrupts as MSI
    // set_ig_msi();

    //# Setup icvec register with an interrupt vector for each cause
    INFO("Setting up interrupt vectors");
    uint64_t icvec = (HPM_INT_VECTOR << 8) | (FQ_INT_VECTOR << 4) | (CQ_INT_VECTOR << 0);
    write64(icvec_addr, icvec);

    //# Configure HPM
    INFO("Configuring HPM");
    // Program event counter registers
    uint64_t iohpmevt[5];
    // iohpmevt[0] = HPM_UT_REQ | 
    //                 ((0xAULL << IOHPMEVT_DID_GSCID_OFF) & (IOHPMEVT_DID_GSCID_MASK)) |
    //                 (IOHPMEVT_DV_GSCV);
    // iohpmevt[1] = HPM_IOTLB_MISS | 
    //                 ((0xBULL << IOHPMEVT_DID_GSCID_OFF) & (IOHPMEVT_DID_GSCID_MASK)) |
    //                 (IOHPMEVT_DV_GSCV);
    // iohpmevt[2] = HPM_DDTW | 
    //                 ((0x0DEFULL << IOHPMEVT_DID_GSCID_OFF) & (IOHPMEVT_DID_GSCID_MASK)) |
    //                 (IOHPMEVT_DV_GSCV) | (IOHPMEVT_IDT);
    // iohpmevt[3] = HPM_S2_PTW | 
    //                 ((0x0AEFULL << IOHPMEVT_DID_GSCID_OFF) & (IOHPMEVT_DID_GSCID_MASK)) |
    //                 (IOHPMEVT_DV_GSCV) | (IOHPMEVT_IDT) | (IOHPMEVT_DMASK);
    iohpmevt[0] = HPM_UT_REQ;
    iohpmevt[1] = HPM_IOTLB_MISS;
    iohpmevt[2] = HPM_DDTW;
    iohpmevt[3] = HPM_S1_PTW;
    iohpmevt[4] = HPM_S2_PTW;

    write64(iohpmevt_addr[0], iohpmevt[0]);
    write64(iohpmevt_addr[1], iohpmevt[1]);
    write64(iohpmevt_addr[2], iohpmevt[2]);
    write64(iohpmevt_addr[3], iohpmevt[3]);
    write64(iohpmevt_addr[4], iohpmevt[4]);

    // Enable counters by writing to iocountinh
    uint32_t iocountinh = (uint32_t)(~(CNT_MASK));    // Enable counters
    write32(iocountihn_addr, iocountinh);

    VERBOSE("IOMMU off | iohgatp: Bare | iosatp: Bare | msiptp: Flat");
}

/**
 *  Set IOMMU OFF. All transactions are disallowed and blocked
 */
void set_iommu_off()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_OFF);

    write64(ddtp_addr, ddtp);
}

void set_iommu_bare()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_BARE);

    write64(ddtp_addr, ddtp);
}

void set_iommu_1lvl()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_1LVL);
    
    write64(ddtp_addr, ddtp);
}

void set_ig_wsi()
{
    uint32_t fctl = (1UL << 1);
    write32(fctl_addr, fctl);
}

void set_ig_msi()
{
    uint32_t fctl = (0UL << 1);
    write32(fctl_addr, fctl);
}