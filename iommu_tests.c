#include <iommu_tests.h>
#include <command_queue.h>
#include <fault_queue.h>
#include <device_contexts.h>
#include <msi_pts.h>
#include <iommu_pts.h>

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

uintptr_t idma_src    = IDMA_REG_ADDR(IDMA_SRC_ADDR);
uintptr_t idma_dest   = IDMA_REG_ADDR(IDMA_DEST_ADDR);
uintptr_t idma_nbytes = IDMA_REG_ADDR(IDMA_N_BYTES);
uintptr_t idma_config = IDMA_REG_ADDR(IDMA_CONFIG);
uintptr_t idma_status = IDMA_REG_ADDR(IDMA_STATUS);
uintptr_t idma_nextid = IDMA_REG_ADDR(IDMA_NEXT_ID);
uintptr_t idma_done   = IDMA_REG_ADDR(IDMA_DONE);

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

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    // Two for the source address from where the iDMA will read values,
    uintptr_t read_paddr1 = phys_page_base(SWITCH1);
    uintptr_t read_paddr2 = phys_page_base(SWITCH2);
    // and others for the destination address where the iDMA will write the read values
    uintptr_t write_paddr1 = phys_page_base(SWITCH1);
    uintptr_t write_paddr2 = phys_page_base(SWITCH2);

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
    uintptr_t idma_src    = IDMA_REG_ADDR(IDMA_SRC_ADDR);
    uintptr_t idma_dest   = IDMA_REG_ADDR(IDMA_DEST_ADDR);
    uintptr_t idma_nbytes = IDMA_REG_ADDR(IDMA_N_BYTES);
    uintptr_t idma_config = IDMA_REG_ADDR(IDMA_CONFIG);
    uintptr_t idma_status = IDMA_REG_ADDR(IDMA_STATUS);
    uintptr_t idma_nextid = IDMA_REG_ADDR(IDMA_NEXT_ID);
    uintptr_t idma_done   = IDMA_REG_ADDR(IDMA_DONE);

    write64(idma_src, (uint64_t)read_paddr1); // Source address
    write64(idma_dest, (uint64_t)write_paddr1);  // Destination address
    write64(idma_nbytes, 8);                     // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // while (read64(idma_status) & 0x1ULL == 1)
    //     ;

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check1, check2 = (read64(write_paddr1) == 0x11);

    /*------------- SECOND TRANSFER --------------*/

    write64(idma_src, (uint64_t)read_paddr2); // Source address
    write64(idma_dest, (uint64_t)write_paddr2);  // Destination address

    while (read64(idma_status) & 0x1ULL == 1)
        ;

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check second transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    check2 = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("iDMA directly connected to system bus", (check1 && check2));

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
    uintptr_t start_raddr1 = phys_page_base(SWITCH1);
    uintptr_t start_raddr2 = phys_page_base(SWITCH2);
    // and others for the destination address where the iDMA will write the read values
    uintptr_t start_waddr1 = phys_page_base(SWITCH1);
    uintptr_t start_waddr2 = phys_page_base(SWITCH2);

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
    uintptr_t idma_src    = IDMA_REG_ADDR(IDMA_SRC_ADDR);
    uintptr_t idma_dest   = IDMA_REG_ADDR(IDMA_DEST_ADDR);
    uintptr_t idma_nbytes = IDMA_REG_ADDR(IDMA_N_BYTES);
    uintptr_t idma_config = IDMA_REG_ADDR(IDMA_CONFIG);
    uintptr_t idma_status = IDMA_REG_ADDR(IDMA_STATUS);
    uintptr_t idma_nextid = IDMA_REG_ADDR(IDMA_NEXT_ID);
    uintptr_t idma_done   = IDMA_REG_ADDR(IDMA_DONE);

    write64(idma_src, (uint64_t)start_raddr1);  // Source address
    write64(idma_dest, (uint64_t)start_waddr1); // Destination address
    write64(idma_nbytes, 24);                   // N of bytes to be transferred
    write64(idma_config, 0 );       // iDMA config

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check1, check2;
    check1 =    ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));

    /*------------- SECOND TRANSFER --------------*/

    write64(idma_src, (uint64_t)start_raddr2); // Source address
    write64(idma_dest, (uint64_t)start_waddr2);  // Destination address

    // while (read64(idma_status) & 0x1ULL == 1)
    //     ;

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check second transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    check2 =    ((read64(start_waddr2     ) == 0x80) &&
                (read64(start_waddr2 + 8 ) == 0x90) &&
                (read64(start_waddr2 + 16) == 0xA0));

    TEST_ASSERT("iDMA directly connected to system bus", (check1 && check2));

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
    VERBOSE("IOMMU off | iohgatp: Bare | iosatp: Bare");
    ddt_init();
    set_iommu_off();

    //# Setup icvec register with an interrupt vector for each cause (CQ and FQ)
    uintptr_t icvec_addr = IOMMU_REG_ADDR(IOMMU_ICVEC_OFFSET);
    uint64_t icvec = (FQ_INT_VECTOR << 4) | (CQ_INT_VECTOR << 0);
    write64(icvec_addr, icvec);
}

/**********************************************************************************************/

bool iommu_off(){

    // Print the name of the test and create test_status variable
    TEST_START();

    //# Set IOMMU off
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
    write64(idma_src, (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes, 8);                    // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check fault record written in memory
    // Read fqh
    uintptr_t fqh_addr = IOMMU_REG_ADDR(IOMMU_FQH_OFFSET);
    uint64_t fqh = read32(fqh_addr);

    // Get address of the next entry in the FQ
    uintptr_t fqb_addr = IOMMU_REG_ADDR(IOMMU_FQB_OFFSET);
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
    uintptr_t ipsr_addr = IOMMU_REG_ADDR(IOMMU_IPSR_OFFSET);
    write32(ipsr_addr, 0x2UL);

    TEST_ASSERT("IOMMU Off: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("IOMMU Off: Recorded IOVA matches with input IOVA", check_iova);

    TEST_END();
}

bool iommu_bare(){

    // Print the name of the test and create test_status variable
    TEST_START();

    //# Set IOMMU to Bare
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

    write64(idma_src, (uint64_t)start_raddr1);  // Source address
    write64(idma_dest, (uint64_t)start_waddr1); // Destination address
    write64(idma_nbytes, 24);                   // N of bytes to be transferred
    write64(idma_config, 0 );                   // iDMA config

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check transfer
    bool check;
    check =    ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));
    TEST_ASSERT("Memory transfer with IOMMU in Bare mode: All values match", check);

    TEST_END();
}

bool both_stages_bare(){

    // Print the name of the test and create test_status variable
    TEST_START();

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

    write64(idma_src, (uint64_t)read_paddr1);   // Source address
    write64(idma_dest, (uint64_t)write_paddr1); // Destination address
    write64(idma_nbytes, 8);                    // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Bare translation: First value matches", check);

    /*------- SECOND TRANSFER -------*/

    write64(idma_src, (uint64_t)read_paddr2);   // Source address
    write64(idma_dest, (uint64_t)write_paddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check second transfer
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Bare translation: Second value matches", check);

    TEST_END();
}

bool second_stage_only(){

    // Print the name of the test and create test_status variable
    TEST_START();

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
    cmd_entry[0]    |= IODIR_DV | (device_id << IODIR_DID_OFF);

    cmd_entry[1]    = 0;

    // Read cqt
    uintptr_t cqt_addr = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cqb_addr = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
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

    write64(idma_src, (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes, 8);                    // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Second-stage only: First transfer matches (Normal translation)", check);

    /*------- SECOND TRANSFER -------*/

    write64(idma_src, (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check second transfer
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Second-stage only: Second transfer matches (MSI translation)", check);

    TEST_END();
}

bool two_stage_translation(){

    // Print the name of the test and create test_status variable
    TEST_START();

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
    cmd_entry[0]    |= IODIR_DV | (device_id << IODIR_DID_OFF);

    cmd_entry[1]    = 0;

    // Read cqt
    uintptr_t cqt_addr = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cqb_addr = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
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

    write64(idma_src, (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes, 8);                    // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Two-stage translation: First transfer matches (4kiB pages)", check);

    /*------- SECOND TRANSFER -------*/

    write64(idma_src, (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check second transfer
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Two-stage translation: Second transfer matches (2MiB superpages)", check);

    /*------- THIRD TRANSFER -------*/

    write64(idma_src, (uint64_t)read_vaddr3);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr3); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check third transfer
    check = (read64(write_paddr3) == 0x33);
    TEST_ASSERT("Two-stage translation: Third transfer matches (1GiB superpages)", check);

    /*------- FOURTH TRANSFER -------*/

    write64(idma_src, (uint64_t)read_vaddr4);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr4); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Check fourth transfer
    check = (read64(write_paddr4) == 0x44);
    TEST_ASSERT("Two-stage translation: Third transfer matches (MSI translation)", check);

    TEST_END();
}

bool iotinval(){

    // Print the name of the test and create test_status variable
    TEST_START();

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
    write64(idma_src, (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes, 8);                    // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //### SECOND TRANSFER
    write64(idma_src, (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
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
    uintptr_t cqt_addr = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cqb_addr = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
    uint64_t cqb = read64(cqb_addr);
    uintptr_t cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);

    //### Perform previous transfers again
    write64(idma_src, (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr1); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    write64(idma_src, (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
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
    cqt_addr = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);
    cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    cqb_addr = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
    cqb = read64(cqb_addr);
    cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);

    //### Perform previous transfers again
    write64(idma_src, (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr1); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    write64(idma_src, (uint64_t)read_vaddr2);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr2); // Destination address

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    // Since we switched first-stage PTEs, 0x22 should have been written in write_paddr1
    check = ((read64(write_paddr2) == 0x22) && (read64(write_paddr1) == 0x11));
    TEST_ASSERT("IOTLB invalidation: Values were swapped again", check);

    TEST_END();
}

bool wsi_generation(){

    // Print the name of the test and create test_status variable
    TEST_START();

    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    //# Configure the IOMMU to generate interrupts as WSI
    uintptr_t fctl_addr = IOMMU_REG_ADDR(IOMMU_FCTL_OFFSET);
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

    write64(idma_src, (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes, 8);                    // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    // Check if ipsr.fip was set in case of error
    uintptr_t ipsr_addr = IOMMU_REG_ADDR(IOMMU_IPSR_OFFSET);
    bool check = (read32(ipsr_addr) == 2);
    TEST_ASSERT("ipsr.fip set on FQ recording", check);

    // Clear ipsr.fip
    write32(ipsr_addr, 0x2UL);

    fence_i();

    check = (read32(ipsr_addr) == 0);
    TEST_ASSERT("ipsr.fip cleared after writing 1", check);

    //# Check fault record written in memory
    // Check CAUSE, TTYP, iotval and iotval2 according to the fault.

    // Read fqh
    uintptr_t fqh_addr = IOMMU_REG_ADDR(IOMMU_FQH_OFFSET);
    uint64_t fqh = read32(fqh_addr);

    // Get address of the next entry in the FQ
    uintptr_t fqb_addr = IOMMU_REG_ADDR(IOMMU_FQB_OFFSET);
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
    uintptr_t cqt_addr = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cqb_addr = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
    uint64_t cqb = read64(cqb_addr);
    uintptr_t cq_entry_base = ((cqb & CQB_PPN_MASK) << 2) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    uintptr_t cqh_addr = IOMMU_REG_ADDR(IOMMU_CQH_OFFSET);
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
    uintptr_t cqcsr_addr = IOMMU_REG_ADDR(IOMMU_CQCSR_OFFSET);
    uint32_t cqcsr = read32(cqcsr_addr);
    check = ((cqcsr & CQCSR_FENCE_W_IP) != 0);
    TEST_ASSERT("cqcsr.fence_w_ip was set", check);

    // Check if ipsr.cip was set for WSI = 1
    uintptr_t ipsr_addr = IOMMU_REG_ADDR(IOMMU_IPSR_OFFSET);
    check = ((read32(ipsr_addr) & CIP_MASK) == 1);
    TEST_ASSERT("ipsr.cip setting on IOFENCE completion", check);

    // Check if data was correctly written in memory for AV = 1
    uintptr_t iofence_addr = IOFENCE_ADDR;
    uint32_t iofence_data = read32(iofence_addr);
    check = (iofence_data == 0x00ABCDEFUL);
    TEST_ASSERT("IOFENCE DATA was correctly written in the given ADDR", check);

    // Clear cqcsr.fence_w_ip and ipsr.cip
    write32(cqcsr_addr, cqcsr);
    write32(ipsr_addr, 0x1UL);

    TEST_END();
}

bool msi_generation(){

    // Print the name of the test and create test_status variable
    TEST_START();

    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");
    VERBOSE("CQ interrupt vector masked");

    //# Configure the IOMMU to generate interrupts as MSI
    uintptr_t fctl_addr = IOMMU_REG_ADDR(IOMMU_FCTL_OFFSET);
    uint32_t fctl = (0UL << 1);
    write32(fctl_addr, fctl);

    //# Configure MSI Config Table
    uintptr_t msi_addr_3_addr       = IOMMU_REG_ADDR(IOMMU_MSI_ADDR_3_OFFSET);
    uintptr_t msi_data_3_addr       = IOMMU_REG_ADDR(IOMMU_MSI_DATA_3_OFFSET);
    uintptr_t msi_vec_ctl_3_addr    = IOMMU_REG_ADDR(IOMMU_MSI_VEC_CTL_3_OFFSET);

    uint64_t msi_addr_3     = 0x83000000ULL;
    uint32_t msi_data_3     = 0x00ABCDEFUL;
    uint32_t msi_vec_ctl_3  = 0x1UL;

    write64(msi_addr_3_addr,    msi_addr_3);
    write32(msi_data_3_addr,    msi_data_3);
    write32(msi_vec_ctl_3_addr, msi_vec_ctl_3);

    // uintptr_t msi_addr_10_addr       = IOMMU_REG_ADDR(IOMMU_MSI_ADDR_10_OFFSET);
    // uintptr_t msi_data_10_addr       = IOMMU_REG_ADDR(IOMMU_MSI_DATA_10_OFFSET);
    // uintptr_t msi_vec_ctl_10_addr    = IOMMU_REG_ADDR(IOMMU_MSI_VEC_CTL_10_OFFSET);

    uintptr_t msi_addr_2_addr       = IOMMU_REG_ADDR(IOMMU_MSI_ADDR_2_OFFSET);
    uintptr_t msi_data_2_addr       = IOMMU_REG_ADDR(IOMMU_MSI_DATA_2_OFFSET);
    uintptr_t msi_vec_ctl_2_addr    = IOMMU_REG_ADDR(IOMMU_MSI_VEC_CTL_2_OFFSET);

    // uint64_t msi_addr_10     = 0x83001000ULL;
    // uint32_t msi_data_10     = 0xFEDCBA00UL;
    // uint32_t msi_vec_ctl_10  = 0x0UL;

    uint64_t msi_addr_2     = 0x83001000ULL;
    uint32_t msi_data_2     = 0xFEDCBA00UL;
    uint32_t msi_vec_ctl_2  = 0x0UL;

    // write64(msi_addr_10_addr,    msi_addr_10);
    // write32(msi_data_10_addr,    msi_data_10);
    // write32(msi_vec_ctl_10_addr, msi_vec_ctl_10);

    write64(msi_addr_2_addr,    msi_addr_2);
    write32(msi_data_2_addr,    msi_data_2);
    write32(msi_vec_ctl_2_addr, msi_vec_ctl_2);

    //# Induce a fault in the FQ with a misconfigured translation
    uintptr_t read_paddr1 = phys_page_base(MSI_GEN_R);
    uintptr_t read_vaddr1 = virt_page_base(MSI_GEN_R);

    uintptr_t write_paddr1 = phys_page_base(MSI_GEN_W);
    uintptr_t write_vaddr1 = virt_page_base(MSI_GEN_W);

    write64(idma_src, (uint64_t)read_vaddr1);   // Source address
    write64(idma_dest, (uint64_t)write_vaddr1); // Destination address
    write64(idma_nbytes, 8);                    // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;

    //# Induce a fault in the CQ
    uint64_t cmd_entry[2];
    cmd_entry[0]    = IOTINVAL | GVMA;

    // Add PSCID (invalid for IOTINVAL.GVMA)
    cmd_entry[0]    |= IOTINVAL_PSCV;

    cmd_entry[1]    = 0;

    // Read cqt
    uintptr_t cqt_addr = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cqb_addr = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
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
    uintptr_t cqcsr_addr = IOMMU_REG_ADDR(IOMMU_CQCSR_OFFSET);
    uint32_t cqcsr = read32(cqcsr_addr);
    bool check = ((cqcsr & CQCSR_CMD_ILL) != 0);
    TEST_ASSERT("cqcsr.cmd_ill was set", check);

    // Check if ipsr.cip and ipsr.fip were set
    uintptr_t ipsr_addr = IOMMU_REG_ADDR(IOMMU_IPSR_OFFSET);
    check = ((read32(ipsr_addr) & (CIP_MASK | FIP_MASK)) == 3);
    TEST_ASSERT("ipsr.cip and ipsr.fip were set", check);

    // Check data for FQ vector
    uint32_t fq_msi_data = read32((uintptr_t)msi_addr_2);
    check = (fq_msi_data == msi_data_2);
    TEST_ASSERT("MSI data corresponding to FQ interrupt vector matches", check);

    // Clear cqcsr.cmd_ill, ipsr.cip and ipsr.fip
    write32(cqcsr_addr, cqcsr);
    write32(ipsr_addr, 0x3UL);

    // Clear mask of CQ interrupt vector
    msi_vec_ctl_3  = 0x0UL;
    write32(msi_vec_ctl_3_addr, msi_vec_ctl_3);

    // Flush cache
    fence_i();

    // Check data for CQ vector after clearing mask
    uint32_t cq_msi_data = read32((uintptr_t)msi_addr_3);
    check = (cq_msi_data == msi_data_3);
    TEST_ASSERT("MSI data corresponding to CQ interrupt vector matches after clearing mask", check);

    TEST_END();
}

