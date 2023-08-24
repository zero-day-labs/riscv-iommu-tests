#include <iommu_tests.h>
#include <command_queue.h>
#include <fault_queue.h>
#include <device_contexts.h>
#include <msi_pts.h>
#include <iommu_pts.h>
#include <hpm.h>

/**
 * !NOTES:
 * 
 *  -   IOMMU Initialization is performed according to the guidelines in the spec document.
 *      Some steps will be skipped in the first implementation of these tests, 
 *      such as reading the capabilities register or the fctl register, since we know its default values.
 * 
 *  -   When using wired-interrupts as IOMMU interrupt generation mechanism, 
 *      the APLIC should be programmed in the initialization phase.
 *      We will perform this step when testing interrupt generation.
 * 
 *  -   The DMA-capable device used to test the IOMMU reads a value from a given address 
 *      in memory and writes it in other position. Thus, known values must be written in memory before
 *      programming the device. In order to facilitate this process, the core MMU will be configured 
 *      in bare mode so we can write directly using physical addresses.
 */

uintptr_t idma_src[N_DMA] = {
    IDMA_REG_ADDR(0, IDMA_SRC_ADDR_OFF),
    IDMA_REG_ADDR(1, IDMA_SRC_ADDR_OFF),
    IDMA_REG_ADDR(2, IDMA_SRC_ADDR_OFF),
    IDMA_REG_ADDR(3, IDMA_SRC_ADDR_OFF)
};

uintptr_t idma_dest[N_DMA] = {
    IDMA_REG_ADDR(0, IDMA_DEST_ADDR_OFF),
    IDMA_REG_ADDR(1, IDMA_DEST_ADDR_OFF),
    IDMA_REG_ADDR(2, IDMA_DEST_ADDR_OFF),
    IDMA_REG_ADDR(3, IDMA_DEST_ADDR_OFF)
};

uintptr_t idma_nbytes[N_DMA] = {
    IDMA_REG_ADDR(0, IDMA_N_BYTES_OFF),
    IDMA_REG_ADDR(1, IDMA_N_BYTES_OFF),
    IDMA_REG_ADDR(2, IDMA_N_BYTES_OFF),
    IDMA_REG_ADDR(3, IDMA_N_BYTES_OFF)
};

uintptr_t idma_config[N_DMA] = {
    IDMA_REG_ADDR(0, IDMA_CONFIG_OFF),
    IDMA_REG_ADDR(1, IDMA_CONFIG_OFF),
    IDMA_REG_ADDR(2, IDMA_CONFIG_OFF),
    IDMA_REG_ADDR(3, IDMA_CONFIG_OFF)
};

uintptr_t idma_status[N_DMA] = {
    IDMA_REG_ADDR(0, IDMA_STATUS_OFF),
    IDMA_REG_ADDR(1, IDMA_STATUS_OFF),
    IDMA_REG_ADDR(2, IDMA_STATUS_OFF),
    IDMA_REG_ADDR(3, IDMA_STATUS_OFF)
};
uintptr_t idma_nextid[N_DMA] = {
    IDMA_REG_ADDR(0, IDMA_NEXT_ID_OFF),
    IDMA_REG_ADDR(1, IDMA_NEXT_ID_OFF),
    IDMA_REG_ADDR(2, IDMA_NEXT_ID_OFF),
    IDMA_REG_ADDR(3, IDMA_NEXT_ID_OFF)
};

uintptr_t idma_done[N_DMA] = {
    IDMA_REG_ADDR(0, IDMA_DONE_OFF),
    IDMA_REG_ADDR(1, IDMA_DONE_OFF),
    IDMA_REG_ADDR(2, IDMA_DONE_OFF),
    IDMA_REG_ADDR(3, IDMA_DONE_OFF)
};

uintptr_t idma_ipsr[N_DMA] = {
    IDMA_REG_ADDR(0, IDMA_IPSR_OFF),
    IDMA_REG_ADDR(1, IDMA_IPSR_OFF),
    IDMA_REG_ADDR(2, IDMA_IPSR_OFF),
    IDMA_REG_ADDR(3, IDMA_IPSR_OFF)
};

uint64_t device_ids[N_DMA] = {
    1ULL,
    2ULL,
    3ULL,
    4ULL
};

// fctl
uintptr_t fctl_addr = IOMMU_REG_ADDR(IOMMU_FCTL_OFFSET);

// CQ
uintptr_t cqcsr_addr = IOMMU_REG_ADDR(IOMMU_CQCSR_OFFSET);
uintptr_t cqb_addr = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
uintptr_t cqh_addr = IOMMU_REG_ADDR(IOMMU_CQH_OFFSET);
uintptr_t cqt_addr = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);

// FQ
uintptr_t fqb_addr = IOMMU_REG_ADDR(IOMMU_FQB_OFFSET);
uintptr_t fqh_addr = IOMMU_REG_ADDR(IOMMU_FQH_OFFSET);

// ipsr
uintptr_t ipsr_addr = IOMMU_REG_ADDR(IOMMU_IPSR_OFFSET);

// icvec
uintptr_t icvec_addr = IOMMU_REG_ADDR(IOMMU_ICVEC_OFFSET);

// HPM
uintptr_t iocountovf_addr   = IOMMU_REG_ADDR(IOMMU_IOCOUNTOVF_OFFSET);
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

// MSI Config data
uint64_t msi_addr_cq     = 0x83000000ULL;
uint32_t msi_data_cq     = 0x00ABCDEFUL;
uint32_t msi_vec_ctl_cq  = 0x1UL;

uint64_t msi_addr_fq     = 0x83001000ULL;
uint32_t msi_data_fq     = 0xFEDCBA00UL;
uint32_t msi_vec_ctl_fq  = 0x0UL;

uint64_t msi_addr_hpm     = 0x83002000ULL;
uint32_t msi_data_hpm     = 0xDEADBEEFUL;
uint32_t msi_vec_ctl_hpm  = 0x0UL;

// HPM global counters
uint8_t ut_reqs, iotlb_misses, ddtw, s2_ptw;

static inline void touchread(uintptr_t addr){
    asm volatile("" ::: "memory");
    volatile uint64_t x = *(volatile uint64_t *)addr;
}

static inline void touchwrite(uintptr_t addr){
    *(volatile uint64_t *)addr = 0xdeadbeef;
}

static inline void touch(uintptr_t addr){
    touchwrite(addr);
}

/**
 *  Test SoC with iDMA module directly connected to the XBAR, i.e., without IOMMU.
 * 
 *  Writes two values in memory, then programs the iDMA to read these values and write
 *  them in a different position in memory
 */
bool idma_only(){

    // Print the name of the test and create test_status variable
    TEST_START();

    bool check;

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    // Two for the source address from where the iDMA will read values,
    uintptr_t read_paddr1 = phys_page_base(BARE_TRANS_R1);
    uintptr_t read_paddr2 = phys_page_base(BARE_TRANS_R2);
    // and others for the destination address where the iDMA will write the read values
    uintptr_t write_paddr1 = phys_page_base(BARE_TRANS_W1);
    uintptr_t write_paddr2 = phys_page_base(BARE_TRANS_W2);

    //# Write known values to memory
    // Write some values to memory using physical addresses (Core MMU in bare mode).
    // The iDMA will read these values using the corresponding physical addresses.
    write64(read_paddr1, 0x11);   // 0x..._0001_0001
    write64(read_paddr2, 0x22);   // 0x..._0010_0010

    write64(write_paddr1, 0x00);   // Clear
    write64(write_paddr2, 0x00);   // Clear

    //# Program the iDMA with Virtual Addresses
    // Program the iDMA with the corresponding VAddresses to read the values that were written 
    // previously in memory, and write them in other VAddresses, whose mapped physical addresses are also known.
    write64(idma_src[0], (uint64_t)read_paddr1); // Source address
    write64(idma_dest[0], (uint64_t)write_paddr1);  // Destination address
    write64(idma_nbytes[0], 8);                     // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check1 = (read64(write_paddr1) == 0x11);

    // check = (read64(idma_ipsr[0]) == 3);
    // TEST_ASSERT("iDMA interrupt pending bits set", check1);

    // write64(idma_ipsr[0], (uint64_t) 0x03);

    // check1 = (read64(idma_ipsr[0]) == 0);
    // TEST_ASSERT("iDMA interrupt pending bits cleared after writing 1", check1);

    /*------------- SECOND TRANSFER --------------*/

    write64(idma_src[1], (uint64_t)read_paddr2); // Source address
    write64(idma_dest[1], (uint64_t)write_paddr2);  // Destination address
    write64(idma_nbytes[1], 8);                     // N of bytes to be transferred
    write64(idma_config[1], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[1]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[1]) != trans_id)
        ;

    //# Check second transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check2 = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Single transfer without IOMMU", (check1 && check2));

    TEST_END();
}

/**
 *  Test multi-beat transfers in the SoC with iDMA module directly connected 
 *  to the XBAR, i.e., without IOMMU.
 * 
 *  Writes eight values in memory, then programs the iDMA to read these values and write
 *  them starting from a different position in memory
 */
bool idma_only_multiple_beats(){

    // Print the name of the test and create test_status variable
    TEST_START();

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    // Two for the source address from where the iDMA will read values,
    uintptr_t start_raddr1 = phys_page_base(IOMMU_OFF_R);
    uintptr_t start_raddr2 = phys_page_base(IOMMU_BARE_R);
    // and others for the destination address where the iDMA will write the read values
    uintptr_t start_waddr1 = phys_page_base(IOMMU_OFF_W);
    uintptr_t start_waddr2 = phys_page_base(IOMMU_BARE_W);

    //# Write known values to memory
    // Write some values to memory using physical addresses (Core MMU in bare mode).
    // The iDMA will read these values using the corresponding physical addresses.
    write64(start_raddr1     , 0x00);
    write64(start_raddr1 + 8 , 0x10);
    write64(start_raddr1 + 16, 0x20);

    write64(start_raddr2     , 0x80);
    write64(start_raddr2 + 8 , 0x90);
    write64(start_raddr2 + 16, 0xA0);

    write64(start_waddr1     , 0x00);
    write64(start_waddr1 + 8 , 0x00);
    write64(start_waddr1 + 16, 0x00);

    write64(start_waddr2     , 0x00);
    write64(start_waddr2 + 8 , 0x00);
    write64(start_waddr2 + 16, 0x00);

    //# Program the iDMA with Virtual Addresses
    // Program the iDMA with the corresponding VAddresses to read the values that were written 
    // previously in memory, and write them in other VAddresses, whose mapped physical addresses are also known.
    write64(idma_src[2], (uint64_t)start_raddr1);  // Source address
    write64(idma_dest[2], (uint64_t)start_waddr1); // Destination address
    write64(idma_nbytes[2], 24);                   // N of bytes to be transferred
    write64(idma_config[2], 0 );       // iDMA config

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[2]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[2]) != trans_id)
        ;


    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check1, check2;
    check1 =   ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));

    /*------------- SECOND TRANSFER --------------*/

    write64(idma_src[3], (uint64_t)start_raddr2); // Source address
    write64(idma_dest[3], (uint64_t)start_waddr2);  // Destination address
    write64(idma_nbytes[3], 24);                   // N of bytes to be transferred
    write64(idma_config[3], 0 );       // iDMA config

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[3]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[3]) != trans_id)
        ;

    //# Check second transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    check2 =   ((read64(start_waddr2     ) == 0x80) &&
                (read64(start_waddr2 + 8 ) == 0x90) &&
                (read64(start_waddr2 + 16) == 0xA0));

    TEST_ASSERT("Multi-beat transfers without IOMMU", (check1 && check2));

    TEST_END();
}

/**
 *  Configure CQ, FQ, S1 and S2 page tables, MSI page tables,
 *  ddtp (IOMMU and translation in bare mode) and interrupt vectors
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
    VERBOSE("CQ: Interrupts enabled");
    cq_init();

    //# Setup the Fault Queue:
    // Allocate a buffer of N (POT) entries (32-bytes each).
    // This buffer must be alligned to the greater of two values: 4-kiB or N x 32 bytes.
    // Configure fqb with the queue size as log2(N) and the base address of the buffer.
    // Set fqh to zero.
    // Enable the FQ by writing 1 to fqcsr.fqen, poll fqcsr.fqon until it reads 1.
    INFO("Configuring FQ");
    VERBOSE("FQ: Interrupts enabled");
    fq_init();

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
    INFO("Configuring MSI page tables");
    msi_pt_init();

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
    uint32_t fctl = (1UL << 1);
    write32(fctl_addr, fctl);

    // //# Configure the IOMMU to generate interrupts as MSI
    // uint32_t fctl = (0UL << 1);
    // write32(fctl_addr, fctl);

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

/**********************************************************************************************/

bool iommu_off(){

    // Print the name of the test and create test_status variable
    TEST_START();

    //# Set IOMMU off
    fence_i();
    set_iommu_off();
    VERBOSE("IOMMU Off");

    //# Get a set of physical address
    uintptr_t read_vaddr1 = virt_page_base(IOMMU_OFF_R);
    uintptr_t read_paddr1 = phys_page_base(IOMMU_OFF_R);
    uintptr_t write_vaddr1 = virt_page_base(IOMMU_OFF_W);
    uintptr_t write_paddr1 = phys_page_base(IOMMU_OFF_W);

    //# Write known values to memory
    write64(read_paddr1, 0x11);
    // Clear destination address
    write64(write_paddr1, 0x00);

    //# Program the iDMA
    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes[0], 8);                    // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check fault record written in memory
    // Read fqh
    uint64_t fqh = read32(fqh_addr);

    // Get address of the next entry in the FQ
    uint64_t fqb = read64(fqb_addr);
    uintptr_t fq_entry_base = ((fqb & FQB_PPN_MASK) << 2) | (fqh << 5);

    // Read FQ record by DWs
    uint64_t fq_entry[4];
    fq_entry[0] = read64(fq_entry_base + 0 );
    fq_entry[1] = read64(fq_entry_base + 8 );
    fq_entry[2] = read64(fq_entry_base + 16);
    fq_entry[3] = read64(fq_entry_base + 24);

    bool check_cause = ((fq_entry[0] & CAUSE_MASK) == ALL_INB_TRANSACTIONS_DISALLOWED);
    bool check_iova  = (fq_entry[2] == read_vaddr1);

    TEST_ASSERT("IOMMU Off: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("IOMMU Off: Recorded IOVA matches with input IOVA", check_iova);

    // Second entry
    fqh++;
    write32(fqh_addr, (uint32_t)fqh);
    fq_entry_base = ((fqb & FQB_PPN_MASK) << 2) | (fqh << 5);

    fq_entry[0] = read64(fq_entry_base + 0 );
    fq_entry[1] = read64(fq_entry_base + 8 );
    fq_entry[2] = read64(fq_entry_base + 16);
    fq_entry[3] = read64(fq_entry_base + 24);

    check_cause = ((fq_entry[0] & CAUSE_MASK) == ALL_INB_TRANSACTIONS_DISALLOWED);
    check_iova  = (fq_entry[2] == write_vaddr1);

    fqh++;
    write32(fqh_addr, (uint32_t)fqh);

    // Clear ipsr.fip
    write32(ipsr_addr, 0x7UL);

    TEST_ASSERT("IOMMU Off: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("IOMMU Off: Recorded IOVA matches with input IOVA", check_iova);

    TEST_END();
}

bool iommu_bare(){

    // Print the name of the test and create test_status variable
    TEST_START();

    //# Set IOMMU to Bare
    fence_i();
    set_iommu_bare();
    VERBOSE("IOMMU in Bare mode");

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    // Two for the source address from where the iDMA will read values,
    uintptr_t start_raddr1 = phys_page_base(IOMMU_BARE_R);
    // and others for the destination address where the iDMA will write the read values
    uintptr_t start_waddr1 = phys_page_base(IOMMU_BARE_W);

    //# Write known values to memory
    write64(start_raddr1     , 0x00);
    write64(start_raddr1 + 8 , 0x10);
    write64(start_raddr1 + 16, 0x20);

    write64(start_waddr1     , 0x00);
    write64(start_waddr1 + 8 , 0x00);
    write64(start_waddr1 + 16, 0x00);

    write64(idma_src[0], (uint64_t)start_raddr1);  // Source address
    write64(idma_dest[0], (uint64_t)start_waddr1); // Destination address
    write64(idma_nbytes[0], 24);                   // N of bytes to be transferred
    write64(idma_config[0], 0 );                   // iDMA config

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check transfer
    bool check;
    check =    ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));
    TEST_ASSERT("Memory transfer with IOMMU in Bare mode: All values match", check);
}

bool both_stages_bare(){

    // Print the name of the test and create test_status variable
    TEST_START();

    fence_i();
    set_iommu_1lvl();
    set_iosatp_bare();
    set_iohgatp_bare();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Bare | iosatp: Bare");

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    // Two for the source address from where the iDMA will read values,
    uintptr_t read_paddr1 = phys_page_base(BARE_TRANS_R1);
    uintptr_t read_paddr2 = phys_page_base(BARE_TRANS_R2);
    // and others for the destination address where the iDMA will write the read values
    uintptr_t write_paddr1 = phys_page_base(BARE_TRANS_W1);
    uintptr_t write_paddr2 = phys_page_base(BARE_TRANS_W2);

    //# Write known values to memory
    write64(read_paddr1, 0x11);
    write64(read_paddr2, 0x22);

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);

    write64(idma_src[0], (uint64_t)read_paddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_paddr1); // Destination address
    write64(idma_nbytes[0], 8);                    // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Bare translation: First value matches", check);

    /*------- SECOND TRANSFER -------*/

    write64(idma_src[0], (uint64_t)read_paddr2);   // Source address
    write64(idma_dest[0], (uint64_t)write_paddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check second transfer
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Bare translation: Second value matches", check);

    TEST_END();
}

bool second_stage_only(){

    // Print the name of the test and create test_status variable
    TEST_START();

    fence_i();
    set_iommu_1lvl();
    set_iosatp_bare();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Bare | msiptp: Flat");

    //# DDTC Invalidation
    // device_id to be invalidated
    uint64_t device_id = DEVICE_ID;
    // Construct command
    uint64_t cmd_entry[2];

    INFO("Writing IODIR.INVAL_DDT to CQ")
    cmd_entry[0]    = IODIR | INVAL_DDT;

    // Add device_id
    // cmd_entry[0]    |= IODIR_DV | (device_id << IODIR_DID_OFF);

    cmd_entry[1]    = 0;

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uint64_t cqb = read64(cqb_addr);
    uintptr_t cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);

    //# Configure iDMA
    uintptr_t read_paddr1 = phys_page_base(S2_ONLY_R);
    uintptr_t read_vaddr1 = virt_page_base(S2_ONLY_R);
    uintptr_t read_paddr2 = phys_page_base(MSI_R1);
    uintptr_t read_vaddr2 = virt_page_base(MSI_R1);
    uintptr_t write_paddr1 = phys_page_base(S2_ONLY_W);
    uintptr_t write_vaddr1 = virt_page_base(S2_ONLY_W);
    uintptr_t write_paddr2 = MSI_BASE_IF_SPA + (17 * PAGE_SIZE);
    uintptr_t write_vaddr2 = virt_page_base(MSI_W1);

    write64(read_paddr1, 0x11);
    write64(read_paddr2, 0x22);

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);

    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes[0], 8);                    // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Second-stage only: First transfer matches (Normal translation)", check);

    /*------- SECOND TRANSFER -------*/
    if (DC_EXT_FORMAT == 1)
    {
        write64(idma_src[0], (uint64_t)read_vaddr2);   // Source address
        write64(idma_dest[0], (uint64_t)write_vaddr2); // Destination address

        // Check if iDMA was set up properly and init transfer
        trans_id = read64(idma_nextid[0]);
        if (!trans_id)
            {ERROR("iDMA misconfigured")}

        // Poll transfer status
        while (read64(idma_done[0]) != trans_id)
            ;

        //# Check second transfer
        check = (read64(write_paddr2) == 0x22);
        TEST_ASSERT("Second-stage only: Second transfer matches (MSI translation)", check);
    }

    TEST_END();
}

bool two_stage_translation(){

    // Print the name of the test and create test_status variable
    TEST_START();

    fence_i();
    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    //# DDTC Invalidation
    // device_id to be invalidated
    uint64_t device_id = DEVICE_ID;
    // Construct command
    uint64_t cmd_entry[2];

    INFO("Writing IODIR.INVAL_DDT to CQ")
    cmd_entry[0]    = IODIR | INVAL_DDT;

    // Add device_id
    // cmd_entry[0]    |= IODIR_DV | (device_id << IODIR_DID_OFF);

    cmd_entry[1]    = 0;

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uint64_t cqb = read64(cqb_addr);
    uintptr_t cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    // Two for the source address from where the iDMA will read values,
    uintptr_t read_paddr1 = phys_page_base(TWO_STAGE_R4K);
    uintptr_t read_vaddr1 = virt_page_base(TWO_STAGE_R4K);
    uintptr_t read_paddr2 = phys_page_base(TWO_STAGE_R2M);
    uintptr_t read_vaddr2 = virt_page_base(TWO_STAGE_R2M);
    uintptr_t read_paddr3 = phys_page_base(TWO_STAGE_R1G);
    uintptr_t read_vaddr3 = virt_page_base(TWO_STAGE_R1G);
    uintptr_t read_paddr4 = phys_page_base(MSI_R2);
    uintptr_t read_vaddr4 = virt_page_base(MSI_R2);
    
    uintptr_t write_paddr1 = phys_page_base(TWO_STAGE_W4K);
    uintptr_t write_vaddr1 = virt_page_base(TWO_STAGE_W4K);
    uintptr_t write_paddr2 = TEST_PADDR_2MIB;
    uintptr_t write_vaddr2 = TEST_VADDR_2MIB;
    uintptr_t write_paddr3 = TEST_PADDR_1GIB;
    uintptr_t write_vaddr3 = TEST_VADDR_1GIB;
    uintptr_t write_paddr4 = MSI_BASE_IF_SPA + (21 * PAGE_SIZE);
    uintptr_t write_vaddr4 = virt_page_base(MSI_W2);

    //# Write known values to memory
    write64(read_paddr1, 0x11);
    write64(read_paddr2, 0x22);
    write64(read_paddr3, 0x33);
    write64(read_paddr4, 0x44);

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);
    write64(write_paddr3, 0x00);
    write64(write_paddr4, 0x00);

    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes[0], 8);                    // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Two-stage translation: First transfer matches (4kiB pages)", check);

    /*------- SECOND TRANSFER -------*/

    write64(idma_src[0], (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check second transfer
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Two-stage translation: Second transfer matches (2MiB superpages)", check);

    /*------- THIRD TRANSFER -------*/

    write64(idma_src[0], (uint64_t)read_vaddr3);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr3); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Check third transfer
    check = (read64(write_paddr3) == 0x33);
    TEST_ASSERT("Two-stage translation: Third transfer matches (1GiB superpages)", check);

    /*------- FOURTH TRANSFER -------*/

    if (DC_EXT_FORMAT == 1)
    {
        write64(idma_src[0], (uint64_t)read_vaddr4);   // Source address
        write64(idma_dest[0], (uint64_t)write_vaddr4); // Destination address

        // Check if iDMA was set up properly and init transfer
        trans_id = read64(idma_nextid[0]);
        if (!trans_id)
            {ERROR("iDMA misconfigured")}

        // Poll transfer status
        while (read64(idma_done[0]) != trans_id)
            ;

        //# Check fourth transfer
        check = (read64(write_paddr4) == 0x44);
        TEST_ASSERT("Two-stage translation: Fourth transfer matches (MSI translation)", check);
    }

    TEST_END();
}

bool iotinval(){

    // Print the name of the test and create test_status variable
    TEST_START();

    fence_i();
    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    uintptr_t read_paddr1 = phys_page_base(IOTINVAL_R1);
    uintptr_t read_vaddr1 = virt_page_base(IOTINVAL_R1);
    uintptr_t read_paddr2 = phys_page_base(IOTINVAL_R2);
    uintptr_t read_vaddr2 = virt_page_base(IOTINVAL_R2);

    uintptr_t write_paddr1 = phys_page_base(SWITCH1);
    uintptr_t write_vaddr1 = virt_page_base(SWITCH1);
    uintptr_t write_paddr2 = phys_page_base(SWITCH2);
    uintptr_t write_vaddr2 = virt_page_base(SWITCH2);

    //# Write known values to memory
    write64(read_paddr1, 0x11);
    write64(read_paddr2, 0x22);

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);

    //### FIRST TRANSFER
    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes[0], 8);                    // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //### SECOND TRANSFER
    write64(idma_src[0], (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check = ((read64(write_paddr1) == 0x11) && (read64(write_paddr2) == 0x22));
    TEST_ASSERT("Memory transfer before PTEs swap", check);

    //### Switch leaf PTEs and clear destination addresses
    INFO("Swapping first-stage page tables")
    s1pt_switch();

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);

    fence_i();

    //### INVALIDATION
    // GSCID tag to be invalidated (16-bits max)
    uint64_t gscid = 0x0ABC;
    // PSCID tag to be invalidated (20-bits max)
    uint64_t pscid = 0x0DEF;

    // Construct command
    uint64_t cmd_entry[2];

    INFO("Writing IOTINVAL.VMA to CQ")
    cmd_entry[0]    = IOTINVAL | VMA;

    // Add GSCID
    cmd_entry[0]    |= IOTINVAL_GV | (gscid << IOTINVAL_GSCID_OFF);

    // Add PSCID
    cmd_entry[0]    |= IOTINVAL_PSCV | (pscid << IOTINVAL_PSCID_OFF);

    cmd_entry[1]    = 0;

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uint64_t cqb = read64(cqb_addr);
    uintptr_t cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);

    //### Perform previous transfers again
    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    write64(idma_src[0], (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    // Since we switched first-stage PTEs, 0x22 should have been written in write_paddr1
    check = ((read64(write_paddr1) == 0x22) && (read64(write_paddr2) == 0x11));
    TEST_ASSERT("IOTLB invalidation: Values were swapped", check);

    INFO("Swapping second-stage page tables")
    s2pt_switch();

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);

    fence_i();

    //### INVALIDATION
    // GSCID tag to be invalidated (16-bits max)
    gscid = 0x0ABC;

    // Construct command
    cmd_entry[2];

    INFO("Writing IOTINVAL.GVMA to CQ")
    cmd_entry[0]    = IOTINVAL | GVMA;

    // Add GSCID
    cmd_entry[0]    |= IOTINVAL_GV | (gscid << IOTINVAL_GSCID_OFF);

    cmd_entry[1]    = 0;

    // Read cqt
    cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    cqb = read64(cqb_addr);
    cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);

    //### Perform previous transfers again
    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    write64(idma_src[0], (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    // Since we switched first-stage PTEs, 0x22 should have been written in write_paddr1
    check = ((read64(write_paddr2) == 0x22) && (read64(write_paddr1) == 0x11));
    TEST_ASSERT("IOTLB invalidation: Values were swapped again", check);

    TEST_END();
}

bool wsi_generation(){

    // Print the name of the test and create test_status variable
    TEST_START();

    fence_i();
    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    //# Configure the IOMMU to generate interrupts as WSI
    uint32_t fctl = (1UL << 1);
    write32(fctl_addr, fctl);

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    uintptr_t read_paddr1 = phys_page_base(WSI_R);
    uintptr_t read_vaddr1 = virt_page_base(WSI_R);

    uintptr_t write_paddr1 = phys_page_base(WSI_W);
    uintptr_t write_vaddr1 = virt_page_base(WSI_W);

    //# Write known values to memory
    write64(read_paddr1, 0x11);

    write64(write_paddr1, 0x00);

    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes[0], 8);                    // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    // Check if ipsr.fip was set in case of error
    bool check = ((read32(ipsr_addr) & FIP_MASK) == 2);
    TEST_ASSERT("ipsr.fip set on FQ recording", check);

    // Clear ipsr.fip
    write32(ipsr_addr, 0x7UL);

    fence_i();

    check = (read32(ipsr_addr) == 0);
    TEST_ASSERT("ipsr.fip cleared after writing 1", check);

    //# Check fault record written in memory
    // Check CAUSE, TTYP, iotval and iotval2 according to the fault.

    // Read fqh
    uint64_t fqh = read32(fqh_addr);

    // Get address of the next entry in the FQ
    uint64_t fqb = read64(fqb_addr);
    uintptr_t fq_entry_base = ((fqb & FQB_PPN_MASK) << 2) | (fqh << 5);

    // Read FQ record by DWs
    uint64_t fq_entry[4];
    fq_entry[0] = read64(fq_entry_base + 0 );
    fq_entry[1] = read64(fq_entry_base + 8 );
    fq_entry[2] = read64(fq_entry_base + 16);
    fq_entry[3] = read64(fq_entry_base + 24);

    // Increment fqh
    fqh++;
    write32(fqh_addr, (uint32_t)fqh);

    bool check_cause = ((fq_entry[0] & CAUSE_MASK) == LOAD_PAGE_FAULT);
    bool check_iova  = (fq_entry[2] == read_vaddr1);

    TEST_ASSERT("Read 1: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("Read 1: Recorded IOVA matches with input IOVA", check_iova);

    // Second entry
    fq_entry_base = ((fqb & FQB_PPN_MASK) << 2) | (fqh << 5);

    fq_entry[0] = read64(fq_entry_base + 0 );
    fq_entry[1] = read64(fq_entry_base + 8 );
    fq_entry[2] = read64(fq_entry_base + 16);
    fq_entry[3] = read64(fq_entry_base + 24);

    // Increment fqh
    fqh++;
    write32(fqh_addr, (uint32_t)fqh);

    check_cause = ((fq_entry[0] & CAUSE_MASK) == STORE_GUEST_PAGE_FAULT);
    check_iova  = (fq_entry[2] == write_vaddr1);

    TEST_ASSERT("Write 1: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("Write 1: Recorded IOVA matches with input IOVA", check_iova);

    TEST_END();
}

bool iofence(){

    // Print the name of the test and create test_status variable
    TEST_START();

    fence_i();
    set_iommu_1lvl();

    //# Construct command
    uint64_t cmd_entry[2];

    INFO("Writing IOFENCE to CQ")
    cmd_entry[0]    = IOFENCE | FUNC3_C;

    // Add WSI
    cmd_entry[0]    |= IOFENCE_WSI;

    // Add AV
    cmd_entry[0]    |= IOFENCE_AV;
    cmd_entry[0]    |= (IOFENCE_DATA << 32);
    cmd_entry[1]    = (IOFENCE_ADDR >> 2);

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uint64_t cqb = read64(cqb_addr);
    uintptr_t cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    uint32_t cqh = read32(cqh_addr);

    // Flush cache
    fence_i();

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);

    // Flush cache
    fence_i();

    uint32_t cqh_inc = read32(cqh_addr);
    bool check = (cqh_inc == (cqh + 1));
    TEST_ASSERT("cqh was incremented", check);

    // Check whether cqcsr.fence_w_ip was set
    uint32_t cqcsr = read32(cqcsr_addr);
    check = ((cqcsr & CQCSR_FENCE_W_IP) != 0);
    TEST_ASSERT("cqcsr.fence_w_ip was set", check);

    // Check if ipsr.cip was set for WSI = 1
    check = ((read32(ipsr_addr) & CIP_MASK) == 1);
    TEST_ASSERT("ipsr.cip setting on IOFENCE completion", check);

    // Check if data was correctly written in memory for AV = 1
    uintptr_t iofence_addr = IOFENCE_ADDR;
    uint32_t iofence_data = read32(iofence_addr);
    check = (iofence_data == 0x00ABCDEFUL);
    TEST_ASSERT("IOFENCE DATA was correctly written in the given ADDR", check);

    // Clear cqcsr.fence_w_ip and ipsr
    write32(cqcsr_addr, cqcsr);
    write32(ipsr_addr, 0x7UL);

    TEST_END();
}

bool msi_generation(){

    // Print the name of the test and create test_status variable
    TEST_START();

    fence_i();
    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");
    VERBOSE("CQ interrupt vector masked");

    //# Configure the IOMMU to generate interrupts as MSI
    uint32_t fctl = (0UL << 1);
    write32(fctl_addr, fctl);

    //# Induce a fault in the FQ with a misconfigured translation
    uintptr_t read_paddr1 = phys_page_base(MSI_GEN_R);
    uintptr_t read_vaddr1 = virt_page_base(MSI_GEN_R);

    uintptr_t write_paddr1 = phys_page_base(MSI_GEN_W);
    uintptr_t write_vaddr1 = virt_page_base(MSI_GEN_W);

    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes[0], 8);                    // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    //# Induce a fault in the CQ
    uint64_t cmd_entry[2];
    cmd_entry[0]    = IOTINVAL | GVMA;

    // Add PSCID (invalid for IOTINVAL.GVMA)
    cmd_entry[0]    |= IOTINVAL_PSCV;

    cmd_entry[1]    = 0;

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uint64_t cqb = read64(cqb_addr);
    uintptr_t cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);

    // Flush cache
    fence_i();

    //# Checks
    // Check whether cqcsr.cmd_ill was set
    uint32_t cqcsr = read32(cqcsr_addr);
    bool check = ((cqcsr & CQCSR_CMD_ILL) != 0);
    TEST_ASSERT("cqcsr.cmd_ill was set", check);

    // Check if ipsr.cip and ipsr.fip were set
    check = ((read32(ipsr_addr) & (CIP_MASK | FIP_MASK)) == 3);
    TEST_ASSERT("ipsr.cip and ipsr.fip were set", check);

    // Check data for FQ vector
    uint32_t fq_msi_data = read32((uintptr_t)msi_addr_fq);
    check = (fq_msi_data == msi_data_fq);
    TEST_ASSERT("MSI data corresponding to FQ interrupt vector matches", check);

    // Clear cqcsr.cmd_ill, ipsr.cip and ipsr.fip
    write32(cqcsr_addr, cqcsr);
    write32(ipsr_addr, 0x7UL);

    // Clear mask of CQ interrupt vector
    msi_vec_ctl_cq  = 0x0UL;
    write32(msi_vec_ctl_cq_addr, msi_vec_ctl_cq);

    // Flush cache
    fence_i();

    // Check data for CQ vector after clearing mask
    uint32_t cq_msi_data = read32((uintptr_t)msi_addr_cq);
    check = (cq_msi_data == msi_data_cq);
    TEST_ASSERT("MSI data corresponding to CQ interrupt vector matches after clearing mask", check);

    TEST_END();
}

bool hpm(){

    // Print the name of the test and create test_status variable
    TEST_START();

    fence_i();
    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    // Flush cache
    fence_i();

    //# Clock counter overflow: WSI
    INFO("Configuring IGS to WSI");
    // Configure the IOMMU to generate interrupts as WSI
    uint32_t fctl = (1UL << 1);
    write32(fctl_addr, fctl);

    // Clear ipsr.fip
    write32(ipsr_addr, 0x7UL);

    // Disable clock counter
    uint32_t iocountinh = read32(iocountihn_addr) | 0x1UL;
    write32(iocountihn_addr, iocountinh);

    // Set iohpmcycles initial value
    uint64_t iohpmcycles = 0x7FFFFFFFFFFFF000ULL;
    write64(iohpmcycles_addr, iohpmcycles);

    // Enable clock counter
    iocountinh &= (~0x1UL);
    write32(iocountihn_addr, iocountinh);

    // Monitor CY bit of iohpmcycles, wait for it to go high
    do
    {
        iohpmcycles = read64(iohpmcycles_addr);
        printf("iohpmcycles value: %llx\n", iohpmcycles);
        for (size_t i = 0; i < 10000; i++)
            ;
    }
    while (!(iohpmcycles & (0x1ULL << 63)));
    
    // Check iocountovf bit
    uint32_t iocountovf = read32(iocountovf_addr);
    bool check = (iocountovf & 0x1UL);
    TEST_ASSERT("iocountovf.cy is set upon iohpmcycles overflow", check);

    // Check ipsr.pmip and corresponding interrupt
    check = ((read32(ipsr_addr) & PMIP_MASK) == 4);
    TEST_ASSERT("ipsr.pmip set upon iohpmcycles overflow", check);

    // Clear ipsr
    write32(ipsr_addr, 0x7UL);

    //# Clock counter overflow: MSI
    // Configure the IOMMU to generate interrupts as MSI
    INFO("Configuring IGS to MSI");
    fctl = (0UL << 1);
    write32(fctl_addr, fctl);

    // Disable clock counter
    iocountinh = read32(iocountihn_addr) | 0x1UL;
    write32(iocountihn_addr, iocountinh);

    // Set iohpmcycles initial value
    // iohpmcycles = 0xFFFFFFFFFFFFF000ULL;
    iohpmcycles = 0x7FFFFFFFFFFFF000ULL;
    write64(iohpmcycles_addr, iohpmcycles);

    // Enable clock counter
    iocountinh &= (~0x1UL);
    write32(iocountihn_addr, iocountinh);

    // Monitor CY bit of iohpmcycles, wait for it to go high
    do
    {
        iohpmcycles = read64(iohpmcycles_addr);
        printf("iohpmcycles value: %llx\n", iohpmcycles);
        for (size_t i = 0; i < 10000; i++)
            ;
    }
    // while ((iohpmcycles & (0x1ULL << 62)));
    while (!(iohpmcycles & (0x1ULL << 63)));
    
    // Check iocountovf bit
    iocountovf = read32(iocountovf_addr);
    check = (iocountovf & 0x1UL);
    TEST_ASSERT("iocountovf.cy is set upon iohpmcycles overflow", check);

    // Check ipsr.pmip and corresponding interrupt
    check = ((read32(ipsr_addr) & PMIP_MASK) == 4);
    TEST_ASSERT("ipsr.pmip set upon iohpmcycles overflow", check);

    // Check data for HPM vector
    uint32_t hpm_msi_data = read32((uintptr_t)msi_addr_hpm);
    check = (hpm_msi_data == msi_data_hpm);
    TEST_ASSERT("MSI data corresponding to HPM interrupt vector matches", check);

    // Clear ipsr
    write32(ipsr_addr, 0x7UL);

    //# Event counter overflow
    INFO("Configuring IGS to WSI");
    // Configure the IOMMU to generate interrupts as WSI
    fctl = (1UL << 1);
    write32(fctl_addr, fctl);

    // Disable iohpmctr[3]
    iocountinh = read32(iocountihn_addr) | 0x10UL;  // Event ctr 3
    write32(iocountihn_addr, iocountinh);

    // Set iohpmctr[3] initial value
    uint64_t iohpmctr = 0xFFFFFFFFFFFFFFFFULL;
    write64(iohpmctr_addr[3], iohpmctr);    // Event ctr 3

    // Enable iohpmctr[3]
    iocountinh &= (~0x10UL);
    write32(iocountihn_addr, iocountinh);

    // Trigger two-stage translation to increment counter
    uintptr_t read_paddr1 = phys_page_base(HPM_R);
    uintptr_t read_vaddr1 = virt_page_base(HPM_R);

    uintptr_t write_paddr1 = phys_page_base(HPM_W);
    uintptr_t write_vaddr1 = virt_page_base(HPM_W);

    write64(idma_src[0], (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest[0], (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes[0], 8);                    // N of bytes to be transferred
    write64(idma_config[0], 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid[0]);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done[0]) != trans_id)
        ;

    // Check iohpmevt.OF
    uint64_t iohpmevt_3 = read64(iohpmevt_addr[3]);
    check = (iohpmevt_3 & IOHPMEVT_OF);
    TEST_ASSERT("iohpmevt_3.OF is set upon iohpmctr[3] overflow", check);

    // Check iocountovf.HPM[3]
    iocountovf = read32(iocountovf_addr);
    check = (iocountovf & 0x10UL);
    TEST_ASSERT("iocountovf.hpm[3] is set upon iohpmctr[3] overflow", check);

    // Check ipsr.pmip and corresponding interrupt
    check = ((read32(ipsr_addr) & PMIP_MASK) == 4);
    TEST_ASSERT("ipsr.pmip set upon iohpmctr[3] overflow", check);

    // Clear ipsr
    write32(ipsr_addr, 0x7UL);

    TEST_END();
}

bool stress_latency(){

    TEST_START();

    int dev_table[16] = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        2
    };

    uint64_t cycles_start = 0;
    uint64_t cycles_end = 0;
    uint64_t cycles_avg = 0;
    uint64_t cycles = 0;

    fence_i();
    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();

    ddt_inval(false, 0);
    iotinval_vma(false, false, false, 0, 0, 0);
    iotinval_gvma(false, false, 0, 0);

    write64(iohpmctr_addr[0], (uint64_t) 0);
    write64(iohpmctr_addr[1], (uint64_t) 0);
    write64(iohpmctr_addr[2], (uint64_t) 0);
    write64(iohpmctr_addr[3], (uint64_t) 0);
    write64(iohpmctr_addr[4], (uint64_t) 0);

    for (size_t i = 0; i < N_TRANSFERS; i++)
    {
        cycles = CSRR(CSR_CYCLES);
        srand(cycles);

        size_t dev_index = rand() % 4;
        size_t read_index = rand() % (STRESS_TOP_RD - STRESS_START_RD);
        size_t write_index = rand() % (STRESS_TOP_WR - STRESS_START_WR);

        //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
        // uintptr_t read_paddr = phys_page_base(read_pages[read_index]);
        // uintptr_t read_vaddr = virt_page_base(read_pages[read_index]);

        // uintptr_t write_paddr = phys_page_base(write_pages[write_index]);
        // uintptr_t write_vaddr = virt_page_base(write_pages[write_index]);

        uintptr_t read_paddr = phys_page_base(read_index + STRESS_START_RD);
        uintptr_t read_vaddr = virt_page_base(read_index + STRESS_START_RD);

        uintptr_t write_paddr = phys_page_base(write_index + STRESS_START_WR);
        uintptr_t write_vaddr = virt_page_base(write_index + STRESS_START_WR);

        fence_i();

        //# Write known values to memory
        write64(read_paddr, 0xDEADBEEF);
        write64(write_paddr, 0);

        write64(idma_src[dev_index], (uint64_t)read_vaddr);   // Source address
        write64(idma_dest[dev_index], (uint64_t)write_vaddr); // Destination address
        write64(idma_nbytes[dev_index], 8);                    // N of bytes to be transferred
        write64(idma_config[dev_index], 0);                    // iDMA config: Disable decouple, deburst and serialize

        // Start stamp
        cycles_start = CSRR(CSR_CYCLES);

        // Check if iDMA was set up properly and init transfer
        uint64_t trans_id = read64(idma_nextid[dev_index]);
        if (!trans_id)
            {ERROR("iDMA misconfigured")}

        // Poll transfer status
        while (read64(idma_done[dev_index]) != trans_id)
            ;

        // End stamp
        cycles_end = CSRR(CSR_CYCLES);

        fence_i();

        if (read64(write_paddr) != 0xDEADBEEF)
            {ERROR("Transfer does not match")}

        cycles_avg += (cycles_end - cycles_start);
    }

    printf("Transfer average latency (in cycles): %llu\n", cycles_avg/N_TRANSFERS);

    printf("Untranslated Requests cnt: %llu\n", read64(iohpmctr_addr[0]));
    printf("IOTLB miss cnt: %llu\n",            read64(iohpmctr_addr[1]));
    printf("DDT Walks cnt: %llu\n",             read64(iohpmctr_addr[2]));
    printf("First-stage PT walk cnt: %llu\n",   read64(iohpmctr_addr[3]));
    printf("Second-stage PT walk cnt: %llu\n",  read64(iohpmctr_addr[4]));

    TEST_END();
}

