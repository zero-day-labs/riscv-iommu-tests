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
    uintptr_t write_paddr1 = phys_page_base(IDMA_WRDEST);
    uintptr_t write_paddr2 = phys_page_base(SCRATCHPAD);

    //# Write known values to memory
    // Write some values to memory using physical addresses (Core MMU in bare mode).
    // The iDMA will read these values using the corresponding physical addresses.
    INFO("Writing known values to memory through physical addresses");
    printf("[%X] = 0x11\n", read_paddr1);
    printf("[%X] = 0x22\n", read_paddr2);
    write64(read_paddr1, 0x11);   // 0x..._0001_0001
    write64(read_paddr2, 0x22);   // 0x..._0010_0010
    INFO("Clearing destination address where iDMA will write");
    printf("[%X] = 0x00\n", write_paddr1);
    printf("[%X] = 0x00\n", write_paddr2);
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

    INFO("Configuring iDMA module for FIRST TRANSFER")
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
    printf("First Transfer ID: %d\n", trans_id);

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;
    INFO("First transfer finished!");

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("iDMA succesfully copied the first value to the desired position in memory", check);

    /*------------- SECOND TRANSFER --------------*/

    INFO("Configuring iDMA module for SECOND TRANSFER")
    write64(idma_src, (uint64_t)read_paddr2); // Source address
    write64(idma_dest, (uint64_t)write_paddr2);  // Destination address

    while (read64(idma_status) & 0x1ULL == 1)
        ;

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}
    printf("Second Transfer ID: %d\n", trans_id);

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;
    INFO("Second transfer finished!")

    //# Check second transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("iDMA succesfully copied the second value to the desired position in memory", check);

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
    uintptr_t start_waddr1 = phys_page_base(IDMA_WRDEST);
    uintptr_t start_waddr2 = phys_page_base(SCRATCHPAD);

    //# Write known values to memory
    // Write some values to memory using physical addresses (Core MMU in bare mode).
    // The iDMA will read these values using the corresponding physical addresses.
    INFO("Writing known values to memory through physical addresses");
    printf("[%X] = 0x00\n", start_raddr1        );
    printf("[%X] = 0x10\n", start_raddr1 + 8    );
    printf("[%X] = 0x20\n", start_raddr1 + 16   );
    // printf("[%X] = 0x30\n", start_raddr1 + 24   );
    // printf("[%X] = 0x40\n", start_raddr1 + 32   );
    // printf("[%X] = 0x50\n", start_raddr1 + 40   );
    // printf("[%X] = 0x60\n", start_raddr1 + 48   );
    // printf("[%X] = 0x70\n", start_raddr1 + 56   );
    printf("\n");
    printf("[%X] = 0x80\n", start_raddr2        );
    printf("[%X] = 0x90\n", start_raddr2 + 8    );
    printf("[%X] = 0xA0\n", start_raddr2 + 16   );
    // printf("[%X] = 0xB0\n", start_raddr2 + 24   );
    // printf("[%X] = 0xC0\n", start_raddr2 + 32   );
    // printf("[%X] = 0xD0\n", start_raddr2 + 40   );
    // printf("[%X] = 0xE0\n", start_raddr2 + 48   );
    // printf("[%X] = 0xF0\n", start_raddr2 + 56   );

    write64(start_raddr1     , 0x00);
    write64(start_raddr1 + 8 , 0x10);
    write64(start_raddr1 + 16, 0x20);
    // write64(start_raddr1 + 24, 0x30);
    // write64(start_raddr1 + 32, 0x40);
    // write64(start_raddr1 + 40, 0x50);
    // write64(start_raddr1 + 48, 0x60);
    // write64(start_raddr1 + 56, 0x70);

    write64(start_raddr2     , 0x80);
    write64(start_raddr2 + 8 , 0x90);
    write64(start_raddr2 + 16, 0xA0);
    // write64(start_raddr2 + 24, 0xB0);
    // write64(start_raddr2 + 32, 0xC0);
    // write64(start_raddr2 + 40, 0xD0);
    // write64(start_raddr2 + 48, 0xE0);
    // write64(start_raddr2 + 56, 0xF0);

    INFO("Clearing destination address where iDMA will write");
    write64(start_waddr1     , 0x00);
    write64(start_waddr1 + 8 , 0x00);
    write64(start_waddr1 + 16, 0x00);
    // write64(start_waddr1 + 24, 0x00);
    // write64(start_waddr1 + 32, 0x00);
    // write64(start_waddr1 + 40, 0x00);
    // write64(start_waddr1 + 48, 0x00);
    // write64(start_waddr1 + 56, 0x00);

    write64(start_waddr2     , 0x00);
    write64(start_waddr2 + 8 , 0x00);
    write64(start_waddr2 + 16, 0x00);
    // write64(start_waddr2 + 24, 0x00);
    // write64(start_waddr2 + 32, 0x00);
    // write64(start_waddr2 + 40, 0x00);
    // write64(start_waddr2 + 48, 0x00);
    // write64(start_waddr2 + 56, 0x00);

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

    INFO("Configuring iDMA module for FIRST TRANSFER")
    write64(idma_src, (uint64_t)start_raddr1);  // Source address
    write64(idma_dest, (uint64_t)start_waddr1); // Destination address
    write64(idma_nbytes, 24);                   // N of bytes to be transferred
    write64(idma_config, 0 );       // iDMA config

    // while (read64(idma_status) & 0x1ULL == 1)
    //     ;

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    printf("First Transfer ID: %d\n", trans_id);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;
    INFO("First transfer finished!");

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check;
    check =    ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));
                // (read64(start_waddr1 + 24) == 0x30) &&
                // (read64(start_waddr1 + 32) == 0x40) &&
                // (read64(start_waddr1 + 40) == 0x50) &&
                // (read64(start_waddr1 + 48) == 0x60) &&
                // (read64(start_waddr1 + 56) == 0x70));
    TEST_ASSERT("iDMA succesfully copied the first values to the desired positions in memory", check);

    /*------------- SECOND TRANSFER --------------*/

    INFO("Configuring iDMA module for SECOND TRANSFER")
    write64(idma_src, (uint64_t)start_raddr2); // Source address
    write64(idma_dest, (uint64_t)start_waddr2);  // Destination address

    // while (read64(idma_status) & 0x1ULL == 1)
    //     ;

    // Check if iDMA was set up properly and init transfer
    trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}
    printf("Second Transfer ID: %d\n", trans_id);

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;
    INFO("Second transfer finished!")

    //# Check second transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    check =    ((read64(start_waddr2     ) == 0x80) &&
                (read64(start_waddr2 + 8 ) == 0x90) &&
                (read64(start_waddr2 + 16) == 0xA0));
                // (read64(start_waddr2 + 24) == 0xB0) &&
                // (read64(start_waddr2 + 32) == 0xC0) &&
                // (read64(start_waddr2 + 40) == 0xD0) &&
                // (read64(start_waddr2 + 48) == 0xE0) &&
                // (read64(start_waddr2 + 56) == 0xF0));
    TEST_ASSERT("iDMA succesfully copied the second value to the desired position in memory", check);

    TEST_END();
}

bool iommu_bare_mode(){

    // Print the name of the test and create test_status variable
    TEST_START();

    // The IOMMU should be in bare mode (ddtp.iommu_mode = 0) by default, 
    // so we should be able to read and write using physical addresses

    //# Setup the Command Queue:
    // Allocate a buffer of N (POT) entries (16-bytes each). 
    // This buffer must be alligned to the greater of two values: 4-kiB or N x 16 bytes.
    // Configure cqb with the queue size as log2(N) and the base address of the buffer.
    // Set cqt to zero.
    // Enable the CQ by writing 1 to cqcsr.cqen, poll cqcsr.cqon until it reads 1.
    // cq_init();

    //# Setup the Fault Queue:
    // Allocate a buffer of N (POT) entries (32-bytes each).
    // This buffer must be alligned to the greater of two values: 4-kiB or N x 32 bytes.
    // Configure fqb with the queue size as log2(N) and the base address of the buffer.
    // Set fqh to zero.
    // Enable the FQ by writing 1 to fqcsr.fqen, poll fqcsr.fqon until it reads 1.
    // fq_init();

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    // Two for the source address from where the iDMA will read values,
    uintptr_t start_raddr1 = phys_page_base(SWITCH1);
    uintptr_t start_raddr2 = phys_page_base(SWITCH2);
    // and others for the destination address where the iDMA will write the read values
    uintptr_t start_waddr1 = phys_page_base(IDMA_WRDEST);
    uintptr_t start_waddr2 = phys_page_base(SCRATCHPAD);

    //# Write known values to memory
    // Write some values to memory using physical addresses (Core MMU in bare mode).
    // The iDMA will read these values using the corresponding physical addresses.
    INFO("Writing known values to memory through physical addresses");
    printf("[%X] = 0x00\n", start_raddr1        );
    printf("[%X] = 0x10\n", start_raddr1 + 8    );
    printf("[%X] = 0x20\n", start_raddr1 + 16   );
    // printf("[%X] = 0x30\n", start_raddr1 + 24   );
    // printf("[%X] = 0x40\n", start_raddr1 + 32   );
    // printf("[%X] = 0x50\n", start_raddr1 + 40   );
    // printf("[%X] = 0x60\n", start_raddr1 + 48   );
    // printf("[%X] = 0x70\n", start_raddr1 + 56   );
    printf("\n");
    printf("[%X] = 0x80\n", start_raddr2        );
    printf("[%X] = 0x90\n", start_raddr2 + 8    );
    printf("[%X] = 0xA0\n", start_raddr2 + 16   );
    // printf("[%X] = 0xB0\n", start_raddr2 + 24   );
    // printf("[%X] = 0xC0\n", start_raddr2 + 32   );
    // printf("[%X] = 0xD0\n", start_raddr2 + 40   );
    // printf("[%X] = 0xE0\n", start_raddr2 + 48   );
    // printf("[%X] = 0xF0\n", start_raddr2 + 56   );

    write64(start_raddr1     , 0x00);
    write64(start_raddr1 + 8 , 0x10);
    write64(start_raddr1 + 16, 0x20);
    // write64(start_raddr1 + 24, 0x30);
    // write64(start_raddr1 + 32, 0x40);
    // write64(start_raddr1 + 40, 0x50);
    // write64(start_raddr1 + 48, 0x60);
    // write64(start_raddr1 + 56, 0x70);

    write64(start_raddr2     , 0x80);
    write64(start_raddr2 + 8 , 0x90);
    write64(start_raddr2 + 16, 0xA0);
    // write64(start_raddr2 + 24, 0xB0);
    // write64(start_raddr2 + 32, 0xC0);
    // write64(start_raddr2 + 40, 0xD0);
    // write64(start_raddr2 + 48, 0xE0);
    // write64(start_raddr2 + 56, 0xF0);

    INFO("Clearing destination address where iDMA will write");
    write64(start_waddr1     , 0x00);
    write64(start_waddr1 + 8 , 0x00);
    write64(start_waddr1 + 16, 0x00);
    // write64(start_waddr1 + 24, 0x00);
    // write64(start_waddr1 + 32, 0x00);
    // write64(start_waddr1 + 40, 0x00);
    // write64(start_waddr1 + 48, 0x00);
    // write64(start_waddr1 + 56, 0x00);

    write64(start_waddr2     , 0x00);
    write64(start_waddr2 + 8 , 0x00);
    write64(start_waddr2 + 16, 0x00);
    // write64(start_waddr2 + 24, 0x00);
    // write64(start_waddr2 + 32, 0x00);
    // write64(start_waddr2 + 40, 0x00);
    // write64(start_waddr2 + 48, 0x00);
    // write64(start_waddr2 + 56, 0x00);

    //# Configure Page Tables for both translation stages in memory
    // Allocate various buffers to work as multi-level page tables.
    // Fill these buffers with leaf and non-leaf entries (pages and superpages)

    /**
     * Setup hyp page_tables.
     */
    INFO("Configuring second-stage page tables");
    goto_priv(PRIV_HS);  // go to HS-mode
    s2pt_init();         // setup hgatp and second-stage PTEs

    /**
     * Setup guest page tables.
     */
    INFO("Configuring first-stage page tables");
    goto_priv(PRIV_VS); // go to VS-mode
    s1pt_init();        // setup satp and first-stage PTEs

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

    //# Program the DDT (ddtp)
    // Assume device_id width same as AXI ID width. Assume DC extended format.
    // Allocate a page of memory to use as the root table of the DDT (Aligned buffer).
    // To do this, determine the number of entries of the root DDT, based on the size of each entry = 4-kiB / 64 bytes.
    // Initialize all entries to zero.
    // Assume support for 3-LVL DDTs.
    // Program the ddtp register with the LVL as the spec specifies and the base address of the page.

    //# Set the DDT with one DC
    // We can use a table with a structure similar to that used for defining different configurations for PTEs.
    // Since we are using a single DMA-capable device to issue reads and writes to memory, 
    // we will only have one entry in the DDT indexed with the AXI ID of the device (device_id).
    // Save the base address of the first-stage root table in DC.iosatp. Same for second-stage and DC.iohgatp.
    // Define an MSI address mask of 5 bits set for the DC.
    // Define an MSI address pattern for the DC, following the format defined in the mask.
    INFO("Configuring DDT");
    ddt_init();

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

    INFO("Configuring iDMA module for FIRST TRANSFER")
    write64(idma_src, (uint64_t)start_raddr1);  // Source address
    write64(idma_dest, (uint64_t)start_waddr1); // Destination address
    write64(idma_nbytes, 24);                   // N of bytes to be transferred
    write64(idma_config, 0 );       // iDMA config

    // while (read64(idma_status) & 0x1ULL == 1)
    //     ;

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    printf("First Transfer ID: %d\n", trans_id);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;
    INFO("First transfer finished!");

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check;
    check =    ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));
                // (read64(start_waddr1 + 24) == 0x30) &&
                // (read64(start_waddr1 + 32) == 0x40) &&
                // (read64(start_waddr1 + 40) == 0x50) &&
                // (read64(start_waddr1 + 48) == 0x60) &&
                // (read64(start_waddr1 + 56) == 0x70));
    TEST_ASSERT("iDMA succesfully copied the first values to the desired positions in memory", check);

    TEST_END();
}

bool both_stages_bare(){

    // Print the name of the test and create test_status variable
    TEST_START();

    // The IOMMU should be in bare mode (ddtp.iommu_mode = 0) by default, 
    // so we should be able to read and write using physical addresses

    //# Setup the Command Queue:
    // Allocate a buffer of N (POT) entries (16-bytes each). 
    // This buffer must be alligned to the greater of two values: 4-kiB or N x 16 bytes.
    // Configure cqb with the queue size as log2(N) and the base address of the buffer.
    // Set cqt to zero.
    // Enable the CQ by writing 1 to cqcsr.cqen, poll cqcsr.cqon until it reads 1.
    INFO("Configuring CQ");
    cq_init();

    //# Setup the Fault Queue:
    // Allocate a buffer of N (POT) entries (32-bytes each).
    // This buffer must be alligned to the greater of two values: 4-kiB or N x 32 bytes.
    // Configure fqb with the queue size as log2(N) and the base address of the buffer.
    // Set fqh to zero.
    // Enable the FQ by writing 1 to fqcsr.fqen, poll fqcsr.fqon until it reads 1.
    INFO("Configuring FQ");
    fq_init();

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    // Two for the source address from where the iDMA will read values,
    uintptr_t read_paddr1 = phys_page_base(SWITCH1);
    uintptr_t read_vaddr1 = virt_page_base(SWITCH1);
    uintptr_t read_paddr2 = phys_page_base(SWITCH2);
    uintptr_t read_vaddr2 = virt_page_base(SWITCH2);
    // and others for the destination address where the iDMA will write the read values
    uintptr_t write_paddr1 = phys_page_base(IDMA_WRDEST);
    uintptr_t write_vaddr1 = virt_page_base(IDMA_WRDEST);
    uintptr_t write_paddr2 = phys_page_base(SCRATCHPAD);
    uintptr_t write_vaddr2 = virt_page_base(SCRATCHPAD);

    //# Write known values to memory
    // Write some values to memory using physical addresses (Core MMU in bare mode).
    // The iDMA will read these values using the corresponding physical addresses.
    INFO("Writing known values to memory through physical addresses");
    printf("[%X] = 0x11\n", read_paddr1);
    printf("[%X] = 0x22\n", read_paddr2);
    write64(read_paddr1, 0x11);   // 0x..._0001_0001
    write64(read_paddr2, 0x22);   // 0x..._0010_0010
    INFO("Clearing destination address where iDMA will write");
    printf("[%X] = 0x00\n", write_paddr1);
    printf("[%X] = 0x00\n", write_paddr2);
    write64(write_paddr1, 0x00);   // Clear
    write64(write_paddr2, 0x00);   // Clear

    //# Configure Page Tables for both translation stages in memory
    // Allocate various buffers to work as multi-level page tables.
    // Fill these buffers with leaf and non-leaf entries (pages and superpages)

    /**
     * Setup hyp page_tables.
     */
    INFO("Configuring second-stage page tables");
    goto_priv(PRIV_HS);  // go to HS-mode
    s2pt_init();         // setup hgatp and second-stage PTEs

    /**
     * Setup guest page tables.
     */
    INFO("Configuring first-stage page tables");
    goto_priv(PRIV_VS); // go to VS-mode
    s1pt_init();        // setup satp and first-stage PTEs

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

    //# Program the DDT (ddtp)
    // Assume device_id width same as AXI ID width. Assume DC extended format.
    // Allocate a page of memory to use as the root table of the DDT (Aligned buffer).
    // To do this, determine the number of entries of the root DDT, based on the size of each entry = 4-kiB / 64 bytes.
    // Initialize all entries to zero.
    // Assume support for 3-LVL DDTs.
    // Program the ddtp register with the LVL as the spec specifies and the base address of the page.

    //# Set the DDT with one DC
    // We can use a table with a structure similar to that used for defining different configurations for PTEs.
    // Since we are using a single DMA-capable device to issue reads and writes to memory, 
    // we will only have one entry in the DDT indexed with the AXI ID of the device (device_id).
    // Save the base address of the first-stage root table in DC.iosatp. Same for second-stage and DC.iohgatp.
    // Define an MSI address mask of 5 bits set for the DC.
    // Define an MSI address pattern for the DC, following the format defined in the mask.
    INFO("Configuring DDT");
    ddt_init();

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

    INFO("Configuring iDMA module for FIRST TRANSFER")
    write64(idma_src, (uint64_t)read_paddr1);   // Source address
    write64(idma_dest, (uint64_t)write_paddr1); // Destination address
    write64(idma_nbytes, 8);                    // N of bytes to be transferred
    write64(idma_config, 0);                    // iDMA config: Disable decouple, deburst and serialize

    // while (read64(idma_status) & 0x1ULL == 1)
    //     ;

    // Check if iDMA was set up properly and init transfer
    uint64_t trans_id = read64(idma_nextid);
    if (!trans_id)
        {ERROR("iDMA misconfigured")}
    printf("First Transfer ID: %d\n", trans_id);

    // Poll transfer status
    while (read64(idma_done) != trans_id)
        ;
    INFO("First transfer finished!");

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("iDMA succesfully copied the first value to the desired position in memory", check);

    TEST_END();
}
