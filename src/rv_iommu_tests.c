#include <rv_iommu_tests.h>
#include <page_tables.h>
#include <rv_iommu.h>
#include <plat_dma.h>
#include <idma.h>

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

// MRIF
extern uint64_t mrif[];

/**
 *  Test SoC with iDMA module directly connected to the XBAR, i.e., without IOMMU.
 * 
 *  Writes two known values in memory.
 *  Then programs the iDMA to read these values and write them in a different position in memory.
 *  Check if the transfer was performed successfully
 */
bool idma_only(){

    // Print the name of the test and create test_status variable
    TEST_START();

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

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
    idma_setup(dma_ut, read_paddr1, write_paddr1, 8);

    // Check if iDMA was set up properly and init transfer
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check1 = (read64(write_paddr1) == 0x11);

    if (IDMA_IRQ_EN)
    {
        check1 = (read64((uintptr_t)&dma_ut->ipsr) == 3);
        TEST_ASSERT("iDMA interrupt pending bits set", check1);

        write64((uintptr_t)&dma_ut->ipsr, (uint64_t) 0x03);

        check1 = (read64((uintptr_t)&dma_ut->ipsr) == 0);
        TEST_ASSERT("iDMA interrupt pending bits cleared after writing 1", check1);
    }

    /*------------- SECOND TRANSFER --------------*/
    idma_setup_addr(dma_ut, read_paddr2, write_paddr2);

    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

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
    
    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

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
    idma_setup(dma_ut, start_raddr1, start_waddr1, 24);

    // Check if iDMA was set up properly and init transfer
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check1, check2;
    check1 =   ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));

    /*------------- SECOND TRANSFER --------------*/

    idma_setup_addr(dma_ut, start_raddr2, start_waddr2);

    // Check if iDMA was set up properly and init transfer
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check second transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    check2 =   ((read64(start_waddr2     ) == 0x80) &&
                (read64(start_waddr2 + 8 ) == 0x90) &&
                (read64(start_waddr2 + 16) == 0xA0));

    TEST_ASSERT("Multi-beat transfers without IOMMU", (check1 && check2));

    TEST_END();
}

/**********************************************************************************************/

/**
 *  Perform a DMA transfer with the IOMMU off
 *  Check fault record written into the FQ
 */
bool iommu_off(){

    TEST_START();

    //# Set IOMMU off
    fence_i();
    set_iommu_off();
    VERBOSE("IOMMU Off");

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    //# Get addresses
    uintptr_t read_vaddr1 = virt_page_base(IOMMU_OFF_R);
    uintptr_t write_vaddr1 = virt_page_base(IOMMU_OFF_W);

    //# Config iDMA and init transfer
    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check fault record written in memory
    uint64_t fq_entry[4];
    if (rv_iommu_fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    bool check_cause = ((fq_entry[0] & CAUSE_MASK) == ALL_INB_TRANSACTIONS_DISALLOWED);
    bool check_iova  = (fq_entry[2] == read_vaddr1);

    TEST_ASSERT("IOMMU Off: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("IOMMU Off: Recorded IOVA matches with input IOVA", check_iova);

    // Second entry
    if (rv_iommu_fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    check_cause = ((fq_entry[0] & CAUSE_MASK) == ALL_INB_TRANSACTIONS_DISALLOWED);
    check_iova  = (fq_entry[2] == write_vaddr1);

    // Clear ipsr.fip
    rv_iommu_clear_ipsr_fip();

    TEST_ASSERT("IOMMU Off: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("IOMMU Off: Recorded IOVA matches with input IOVA", check_iova);

    TEST_END();
}

/**
 *  Carry out a multi-beat DMA transfer with the IOMMU in bare mode, using physical addresses
 *  Check if all values are written correctly
 */
bool iommu_bare(){

    TEST_START();

    //# Set IOMMU to Bare
    fence_i();
    set_iommu_bare();
    VERBOSE("IOMMU in Bare mode");

    /** Map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];
    
    //# Get addresses
    uintptr_t start_raddr1 = phys_page_base(IOMMU_BARE_R);
    uintptr_t start_waddr1 = phys_page_base(IOMMU_BARE_W);

    //# Write known values to memory
    write64(start_raddr1     , 0x00);
    write64(start_raddr1 + 8 , 0x10);
    write64(start_raddr1 + 16, 0x20);

    write64(start_waddr1     , 0x00);
    write64(start_waddr1 + 8 , 0x00);
    write64(start_waddr1 + 16, 0x00);

    //# Configure DMA
    idma_setup(dma_ut, start_raddr1, start_waddr1, 24);
    //# Initiate DMA transfer (only returns when transfer is done)
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check transfer
    bool check;
    check =    ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));
    TEST_ASSERT("Memory transfer with IOMMU in Bare mode: All values match", check);
}

/**
 *  Carry out two DMA transfer with the IOMMU in 1LVL mode
 *  First and second-stage translation are in bare mode. We use physical addresses
 *  Check if all values are written correctly
 */
bool both_stages_bare(){

    TEST_START();
    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_bare();
    rv_iommu_set_iohgatp_bare();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Bare | iosatp: Bare");

    //# Get addresses
    uintptr_t read_paddr1 = phys_page_base(BARE_TRANS_R1);
    uintptr_t read_paddr2 = phys_page_base(BARE_TRANS_R2);
    uintptr_t write_paddr1 = phys_page_base(BARE_TRANS_W1);
    uintptr_t write_paddr2 = phys_page_base(BARE_TRANS_W2);

    //# Write known values to memory
    write64(read_paddr1, 0x11);
    write64(read_paddr2, 0x22);

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);

    //# Config iDMA and init transfer
    idma_setup(dma_ut, read_paddr1, write_paddr1, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Bare translation: First value matches", check);

    /*------- SECOND TRANSFER -------*/

    //# Config iDMA and init transfer
    idma_setup_addr(dma_ut, read_paddr2, write_paddr2);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check second transfer
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Bare translation: Second value matches", check);

    TEST_END();
}

/**
 *  Perform two DMA transfers with the IOMMU in 1LVL mode
 *  Second-stage translation is Sv39x4, first-stage is bare
 *  We also issue IODIR.INVAL_DDT command
 *  Second transfer triggers MSI translation, if supported
 */
bool second_stage_only(){

    TEST_START();
    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_bare();
    rv_iommu_set_iohgatp_sv39x4();
    rv_iommu_set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Bare | msiptp: Flat");

    //# DDTC Invalidation
    rv_iommu_ddt_inval(false, idma_ids[idma_idx]);

    //# Get addresses
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

    //# Config iDMA and init transfer
    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Second-stage only: First transfer matches (Normal translation)", check);

    /*------- SECOND TRANSFER (if MSI translation is supported) -------*/
    if (MSI_TRANSLATION == 1)
    {
        //# Config iDMA and init transfer
        idma_setup(dma_ut, read_vaddr2, write_vaddr2, 4);
        if (idma_exec_transfer(dma_ut) != 0)
            {ERROR("iDMA misconfigured")}

        //# Check second transfer
        check = (read64(write_paddr2) == 0x22);
        TEST_ASSERT("Second-stage only: Second transfer matches (MSI translation)", check);
    }

    TEST_END();
}

/**
 *  Perform two DMA transfers with the IOMMU in 1LVL mode
 *  Second-stage translation is Sv39x4, first-stage is Sv39
 *  We also issue IODIR.INVAL_DDT command
 *  First transfer is 4-kiB
 *  Second transfer is 2-MiB
 *  Third transfer is 1-GiB
 *  Fourth transfer triggers normal first-stage translation, and MSI translation
 */
bool two_stage_translation(){

    TEST_START();

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];
    
    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_sv39();
    rv_iommu_set_iohgatp_sv39x4();
    rv_iommu_set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    //# DDTC Invalidation
    rv_iommu_ddt_inval(false, idma_ids[idma_idx]);

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
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
    uintptr_t write_paddr4 = MSI_BASE_IF_SPA + (19 * PAGE_SIZE);
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

    //# Config iDMA and init transfer
    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Two-stage translation: First transfer matches (4kiB pages)", check);

    /*------- SECOND TRANSFER -------*/

    //# Config iDMA and init transfer
    idma_setup_addr(dma_ut, read_vaddr2, write_vaddr2);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check second transfer
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Two-stage translation: Second transfer matches (2MiB superpages)", check);

    /*------- THIRD TRANSFER -------*/

    //# Config iDMA and init transfer
    idma_setup_addr(dma_ut, read_vaddr3, write_vaddr3);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check third transfer
    check = (read64(write_paddr3) == 0x33);
    TEST_ASSERT("Two-stage translation: Third transfer matches (1GiB superpages)", check);

    /*------- FOURTH TRANSFER (If MSI translation supported) -------*/

    if (MSI_TRANSLATION == 1)
    {
        //# Config iDMA and init transfer
        idma_setup(dma_ut, read_vaddr4, write_vaddr4, 4);
        if (idma_exec_transfer(dma_ut) != 0)
            {ERROR("iDMA misconfigured")}

        //# Check fourth transfer
        check = (read64(write_paddr4) == 0x44);
        TEST_ASSERT("Two-stage translation: Fourth transfer matches (MSI translation)", check);
    }

    TEST_END();
}

/**
 *  Test IOTINVAL.VMA and IOTINVAL.GVMA invalidation commands
 *  We perform three sets of two transfers:
 *    - The first one is to fill the IOTLB
 *    - We then switch first-stage PTs and invalidate first-stage data in the IOTLB. 
 *      After performing the same transfers again, data is written in switched destinations.
 *    - Subsequently, we switch second-stage PTs, and invalidate second-stage data in the IOTLB.
 *      After performing the same transfers once again, data is written as in the first set.
 */
bool iotinval(){

    TEST_START();

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_sv39();
    rv_iommu_set_iohgatp_sv39x4();
    rv_iommu_set_msi_flat();
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
    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    //### SECOND TRANSFER
    idma_setup_addr(dma_ut, read_vaddr2, write_vaddr2);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check = ((read64(write_paddr1) == 0x11) && (read64(write_paddr2) == 0x22));
    TEST_ASSERT("Memory transfer before PTEs swap", check);

    //### Switch first-stage PTEs and clear destination addresses
    INFO("Swapping first-stage page tables")
    s1pt_switch();

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);

    fence_i();

    //### IOTINVAL.VMA
    rv_iommu_iotinval_vma(false, true, true, 0, 0xABC, 0xDEF);

    //### Perform previous transfers again
    idma_setup_addr(dma_ut, read_vaddr1, write_vaddr1);  // first
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    idma_setup_addr(dma_ut, read_vaddr2, write_vaddr2);  // second
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    // Since we switched first-stage PTEs, 0x22 should have been written in write_paddr1
    check = ((read64(write_paddr1) == 0x22) && (read64(write_paddr2) == 0x11));
    TEST_ASSERT("IOTLB invalidation: Values were swapped", check);

    //### Switch second-stage PTEs and clear destination addresses
    INFO("Swapping second-stage page tables")
    s2pt_switch();

    write64(write_paddr1, 0x00);
    write64(write_paddr2, 0x00);

    fence_i();

    //### IOTINVAL.GVMA
    rv_iommu_iotinval_gvma(false, true, 0, 0xABC);

    //### Perform previous transfers again
    idma_setup_addr(dma_ut, read_vaddr1, write_vaddr1);  // first
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    idma_setup_addr(dma_ut, read_vaddr2, write_vaddr2);  // second
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    // Since we switched first-stage PTEs, 0x22 should have been written in write_paddr1
    check = ((read64(write_paddr2) == 0x22) && (read64(write_paddr1) == 0x11));
    TEST_ASSERT("IOTLB invalidation: Values were swapped again", check);

    TEST_END();
}

/**
 *  Carry out a misconfigured DMA transfer to generate a FQ record, and a WSI
 */
bool wsi_generation(){

    TEST_START();

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_sv39();
    rv_iommu_set_iohgatp_sv39x4();
    rv_iommu_set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    //# Configure the IOMMU to generate interrupts as WSI
    set_ig_wsi();

    //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
    uintptr_t read_paddr1 = phys_page_base(WSI_R);
    uintptr_t read_vaddr1 = virt_page_base(WSI_R);

    uintptr_t write_paddr1 = phys_page_base(WSI_W);
    uintptr_t write_vaddr1 = virt_page_base(WSI_W);

    //# Write known values to memory
    write64(read_paddr1, 0x11);
    write64(write_paddr1, 0x00);

    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    // Check if ipsr.fip was set in case of error
    bool check = ((rv_iommu_get_ipsr() & FIP_MASK) == 2);
    TEST_ASSERT("ipsr.fip set on FQ recording", check);

    // Clear ipsr.fip
    rv_iommu_clear_ipsr_fip();

    fence_i();

    check = (rv_iommu_get_ipsr() == 0);
    TEST_ASSERT("ipsr.fip cleared after writing 1", check);

    //# Check fault record written in memory
    // Check CAUSE, TTYP, iotval and iotval2 according to the fault.
    // Read FQ record by DWs
    uint64_t fq_entry[4];
    if (rv_iommu_fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    bool check_cause = ((fq_entry[0] & CAUSE_MASK) == LOAD_PAGE_FAULT);
    bool check_iova  = (fq_entry[2] == read_vaddr1);

    TEST_ASSERT("Read 1: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("Read 1: Recorded IOVA matches with input IOVA", check_iova);

    // Second entry
    if (rv_iommu_fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    check_cause = ((fq_entry[0] & CAUSE_MASK) == STORE_GUEST_PAGE_FAULT);
    check_iova  = (fq_entry[2] == write_vaddr1);

    TEST_ASSERT("Write 1: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("Write 1: Recorded IOVA matches with input IOVA", check_iova);

    TEST_END();
}

/**
 *  Issue an IOFENCE.C command, with WSI and AV set to 1.
 *  Check MSI transfer and fence_w_ip bit 
 */
bool iofence(){

    TEST_START();

    fence_i();
    set_iommu_1lvl();

    uint32_t cqh = rv_iommu_get_cqh();

    //# Send command
    rv_iommu_iofence_c(true, true);

    // Flush cache
    fence_i();

    uint32_t cqh_inc = rv_iommu_get_cqh();
    bool check = (cqh_inc == (cqh + 1));
    TEST_ASSERT("cqh was incremented", check);

    // Check whether cqcsr.fence_w_ip was set
    uint32_t cqcsr = rv_iommu_get_cqcsr();
    check = ((cqcsr & CQCSR_FENCE_W_IP) != 0);
    TEST_ASSERT("cqcsr.fence_w_ip was set", check);

    // Check if ipsr.cip was set for WSI = 1
    check = ((rv_iommu_get_ipsr() & CIP_MASK) == 1);
    TEST_ASSERT("ipsr.cip setting on IOFENCE completion", check);

    // Check if data was correctly written in memory for AV = 1
    uint32_t iofence_data = rv_iommu_get_iofence();
    check = (iofence_data == IOFENCE_DATA);
    TEST_ASSERT("IOFENCE DATA was correctly written in the given ADDR", check);

    // Clear cqcsr.fence_w_ip and ipsr
    rv_iommu_set_cqcsr(cqcsr);
    rv_iommu_clear_ipsr_fip();

    TEST_END();
}

/**
 *  Induce a fault in the IOMMU with a misconfigured translation.
 *  Also induce a fault in the CQ with a misconfigured command.
 */
bool msi_generation(){

    // Print the name of the test and create test_status variable
    TEST_START();

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];
    
    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_sv39();
    rv_iommu_set_iohgatp_sv39x4();
    rv_iommu_set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");
    VERBOSE("CQ interrupt vector masked");

    //# Configure the IOMMU to generate interrupts as MSI
    set_ig_msi();

    //# Induce a fault in the FQ with a misconfigured translation
    uintptr_t read_vaddr1 = virt_page_base(MSI_GEN_R);
    uintptr_t write_vaddr1 = virt_page_base(MSI_GEN_W);

    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}


    rv_iommu_induce_fault_cq();

    // Flush cache
    fence_i();

    //# Checks
    // Check whether cqcsr.cmd_ill was set
    uint32_t cqcsr = rv_iommu_get_cqcsr();
    bool check = ((cqcsr & CQCSR_CMD_ILL) != 0);
    TEST_ASSERT("cqcsr.cmd_ill was set", check);

    // Check if ipsr.cip and ipsr.fip were set
    check = ((rv_iommu_get_ipsr() & (CIP_MASK | FIP_MASK)) == 3);
    TEST_ASSERT("ipsr.cip and ipsr.fip were set", check);

    // Check data for FQ vector
    uint32_t fq_msi_data = read32((uintptr_t)MSI_ADDR_FQ);
    check = (fq_msi_data == MSI_DATA_FQ);
    TEST_ASSERT("MSI data corresponding to FQ interrupt vector matches", check);

    // Clear cqcsr.cmd_ill, ipsr.cip and ipsr.fip
    rv_iommu_set_cqcsr(cqcsr);
    rv_iommu_clear_ipsr_fip();

    // Clear mask of CQ interrupt vector
    rv_iommu_set_msi_cfg_tbl_vctl(3, 0x0UL);

    // Flush cache
    fence_i();

    // Check data for CQ vector after clearing mask
    uint32_t cq_msi_data = read32((uintptr_t)MSI_ADDR_CQ);
    check = (cq_msi_data == MSI_DATA_CQ);
    TEST_ASSERT("MSI data corresponding to CQ interrupt vector matches after clearing mask", check);

    // Discard fault records written in memory
    uint64_t fq_entry[4];
    if (rv_iommu_fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    if (rv_iommu_fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    TEST_END();
}

/**
 *  Enable free clock cycles counter and verify overflow bit.
 *  Verify interrupt (MSI/WSI).
 *  Enable event counter programmed for IOTLB misses.
 *  Verify overflow and interrupt
 */
bool hpm(){

    // Print the name of the test and create test_status variable
    TEST_START();
    
    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_sv39();
    rv_iommu_set_iohgatp_sv39x4();
    rv_iommu_set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    // Flush cache
    fence_i();

    //# Clock counter overflow: WSI
    INFO("Configuring IGS to WSI");
    set_ig_wsi();

    // Clear ipsr.fip
    rv_iommu_clear_ipsr_fip();

    // Disable clock counter
    uint32_t iocountinh = rv_iommu_get_iocountihn() | 0x1UL;
    rv_iommu_set_iocountihn(iocountinh);

    // Set iohpmcycles initial value
    // We set it high to force it to overflow
    uint64_t iohpmcycles = 0x7FFFFFFFFFFFF000ULL;
    rv_iommu_set_iohpmcycles(iohpmcycles);

    // Enable clock counter
    iocountinh &= (~0x1UL);
    rv_iommu_set_iocountihn(iocountinh);

    // Monitor CY bit of iohpmcycles, wait for it to go high
    do
    {
        iohpmcycles = rv_iommu_get_iohpmcycles();
        printf("iohpmcycles value: %llx\n", iohpmcycles);
    }
    while (!(iohpmcycles & (0x1ULL << 63)));
    
    // Check iocountovf bit
    uint32_t iocountovf = rv_iommu_get_iocountovf();
    bool check = (iocountovf & 0x1UL);
    TEST_ASSERT("iocountovf.cy is set upon iohpmcycles overflow", check);

    // Check ipsr.pmip and corresponding interrupt
    check = ((rv_iommu_get_ipsr() & PMIP_MASK) == 4);
    TEST_ASSERT("ipsr.pmip set upon iohpmcycles overflow", check);

    // Clear ipsr
    rv_iommu_clear_ipsr_fip();

    //# Clock counter overflow: MSI
    // Configure the IOMMU to generate interrupts as MSI
    INFO("Configuring IGS to MSI");
    set_ig_msi();

    // Disable clock counter
    iocountinh = rv_iommu_get_iocountihn() | 0x1UL;
    rv_iommu_set_iocountihn(iocountinh);

    // Set iohpmcycles initial value
    iohpmcycles = 0x7FFFFFFFFFFFF000ULL;
    rv_iommu_set_iohpmcycles(iohpmcycles);

    // Enable clock counter
    iocountinh &= (~0x1UL);
    rv_iommu_set_iocountihn(iocountinh);

    // Monitor CY bit of iohpmcycles, wait for it to go high
    do
    {
        iohpmcycles = rv_iommu_get_iohpmcycles();
        printf("iohpmcycles value: %llx\n", iohpmcycles);
        for (size_t i = 0; i < 10000; i++)
            ;
    }
    // while ((iohpmcycles & (0x1ULL << 62)));
    while (!(iohpmcycles & (0x1ULL << 63)));
    
    // Check iocountovf bit
    iocountovf = rv_iommu_get_iocountovf();
    check = (iocountovf & 0x1UL);
    TEST_ASSERT("iocountovf.cy is set upon iohpmcycles overflow", check);

    // Check ipsr.pmip and corresponding interrupt
    check = ((rv_iommu_get_ipsr() & PMIP_MASK) == 4);
    TEST_ASSERT("ipsr.pmip set upon iohpmcycles overflow", check);

    // Check data for HPM vector
    uint32_t hpm_msi_data = read32((uintptr_t)MSI_ADDR_HPM);
    check = (hpm_msi_data == MSI_DATA_HPM);
    TEST_ASSERT("MSI data corresponding to HPM interrupt vector matches", check);

    // Clear ipsr
    rv_iommu_clear_ipsr_fip();

    //# Event counter overflow
    INFO("Configuring IGS to WSI");
    // Configure the IOMMU to generate interrupts as WSI
    set_ig_wsi();

    // Disable iohpmctr[3]
    iocountinh = rv_iommu_get_iocountihn() | 0x10UL;  // Event ctr 3
    rv_iommu_set_iocountihn(iocountinh);

    // Set iohpmctr[3] initial value
    uint64_t iohpmctr = 0xFFFFFFFFFFFFFFFFULL;
    rv_iommu_set_iohpmctr(iohpmctr, 3);

    // Enable iohpmctr[3]
    iocountinh &= (~0x10UL);
    rv_iommu_set_iocountihn(iocountinh);

    // Trigger two-stage translation to increment counter
    uintptr_t read_paddr1 = phys_page_base(HPM_R);
    uintptr_t read_vaddr1 = virt_page_base(HPM_R);

    uintptr_t write_paddr1 = phys_page_base(HPM_W);
    uintptr_t write_vaddr1 = virt_page_base(HPM_W);

    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}

    // Check iohpmevt.OF
    uint64_t iohpmevt_3 = rv_iommu_get_iohpmevt(3);
    check = (iohpmevt_3 & IOHPMEVT_OF);
    TEST_ASSERT("iohpmevt_3.OF is set upon iohpmctr[3] overflow", check);

    // Check iocountovf.HPM[3]
    iocountovf = rv_iommu_get_iocountovf();
    check = (iocountovf & 0x10UL);
    TEST_ASSERT("iocountovf.hpm[3] is set upon iohpmctr[3] overflow", check);

    // Check ipsr.pmip and corresponding interrupt
    check = ((rv_iommu_get_ipsr() & PMIP_MASK) == 4);
    TEST_ASSERT("ipsr.pmip set upon iohpmctr[3] overflow", check);

    // Clear ipsr
    rv_iommu_clear_ipsr_fip();

    TEST_END();
}


/**
 *  MRIF support for MSI translation
 * 
 *  TEST 1
 *  Trigger a second-stage-only address translation using a GPA that maps to an MSI PTE in MRIF mode
 *  The data of this transfer is the interrupt ID of an MRIF with IE set
 *  The write of the MSI notice is verified
 * 
 *  TEST 2
 *  Trigger a two-stage address translation using a GVA->GPA mapping that maps to an MSI PTE in MRIF mode
 *  The data of this transfer is the interrupt ID of an MRIF with IE clear. Thus, the MSI notice is not written
 * 
 *  TEST 3
 *  Trigger an MSI translation with an MSI PTE in MRIF mode, and custom bit set (misconfigured)
 *  Check fault record written to the FQ
 * 
 *  TEST 4
 *  Repeat the same transfer of TEST 2, but with an invalid interrupt identity (bits [31:11] not zero)
 */
bool mrif_support(){

    TEST_START();

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    //# TEST 1: MRIF transaction with second-stage only and MSI notice write
    // Configure data structures and IOMMU
    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_bare();
    rv_iommu_set_iohgatp_sv39x4();
    rv_iommu_set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Bare | msiptp: Flat");

    // DDTC Invalidation
    rv_iommu_ddt_inval(false, idma_ids[idma_idx]);

    // Get addresses
    uintptr_t read_paddr1 = phys_page_base(MSI_R3);
    uintptr_t read_vaddr1 = virt_page_base(MSI_R3);

    uintptr_t write_vaddr1 = virt_page_base(MSI_W3);

    write32(read_paddr1, INT_ID_1);
    write32((uintptr_t)NOTICE_ADDR_1, 0);

    // Config iDMA and init transfer
    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 4);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}
    
    for (size_t i = 0; i < 100; i++)
        ;
    
    // Check IP bit
    bool check = ((mrif[INT_IP_IDX_1] & INT_MASK_1) != 0);
    TEST_ASSERT("MRIF with Second-stage only: IP bit set", check);
    // Check MSI notice
    check = (read32(NOTICE_ADDR_1) == NOTICE_DATA);
    TEST_ASSERT("MRIF with Second-stage only: MSI notice matches", check);

    //# TEST 2: MRIF transaction with two-stage and IE disabled (no MSI notice)
    // Enable first-stage translation
    rv_iommu_set_iosatp_sv39();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    // DDTC Invalidation
    rv_iommu_ddt_inval(false, idma_ids[idma_idx]);

    // Get addresses
    uintptr_t read_paddr2 = phys_page_base(MSI_R4);
    uintptr_t read_vaddr2 = virt_page_base(MSI_R4);

    uintptr_t write_vaddr2 = virt_page_base(MSI_W4);

    write32(read_paddr2, INT_ID_2);
    write32((uintptr_t)NOTICE_ADDR_2, 0);

    // Config iDMA and init transfer
    idma_setup(dma_ut, read_vaddr2, write_vaddr2, 4);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}
    
    for (size_t i = 0; i < 100; i++)
        ;
    
    // Check IP bit
    check = ((mrif[INT_IP_IDX_2] & INT_MASK_2) != 0);
    TEST_ASSERT("MRIF with two-stage translation: IP bit set", check);
    // Check MSI notice (not written)
    check = (read32(NOTICE_ADDR_2) == 0);
    TEST_ASSERT("MRIF with two-stage translation: MSI notice not written as IE is clear", check);

    //# TEST 3: MRIF transaction with misconfigured MSI PTE (custom bit set)
    // Get addresses
    uintptr_t read_vaddr3 = virt_page_base(MSI_R5);

    uintptr_t write_vaddr3 = virt_page_base(MSI_W5);

    // No need to configure data in physical addresses as transfer will fail

    // Config iDMA and init transfer
    idma_setup(dma_ut, read_vaddr3, write_vaddr3, 4);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}
    
    for (size_t i = 0; i < 100; i++)
        ;
    
    // Check fault record written in memory
    uint64_t fq_entry[4];
    if (rv_iommu_fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    bool check_cause = ((fq_entry[0] & CAUSE_MASK) == MSI_PTE_INVALID);
    bool check_iova  = (fq_entry[2] == write_vaddr3);

    TEST_ASSERT("MRIF: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("MRIF: Recorded IOVA matches with input IOVA", check_iova);

    // Clear ipsr.fip
    rv_iommu_clear_ipsr_fip();

    //# TEST 4: Transaction discarding mechanism using an invalid interrupt ID

    fence_i();
    write32(read_paddr1, 0xFFFUL);          // Invalid int. ID
    mrif[INT_IP_IDX_1] = 0;                 // clear IP
    write32((uintptr_t)NOTICE_ADDR_1, 0);   // clear MSI notice

    // Config iDMA and init transfer
    idma_setup(dma_ut, read_vaddr1, write_vaddr1, 4);
    if (idma_exec_transfer(dma_ut) != 0)
        {ERROR("iDMA misconfigured")}
    
    for (size_t i = 0; i < 100; i++)
        ;
    
    // Check IP bit
    check = ((mrif[INT_IP_IDX_1] & INT_MASK_1) == 0);
    TEST_ASSERT("MRIF discard mechanism: IP bit not set", check);
    // Check MSI notice (not written)
    check = (read32(NOTICE_ADDR_1) == 0);
    TEST_ASSERT("MRIF discard mechanism: MSI notice not written as transaction was discarded", check);

    TEST_END();
}

/**
 *  Debug Register Interface
 * 
 *  TEST 1
 *  4kiB translation
 * 
 *  TEST 2
 *  2MiB translation
 * 
 *  TEST 3
 *  1GiB translation
 * 
 *  TEST 4
 *  MSI translation using DBG IF (Error propagation)
 * 
 */
bool dbg_interface(){

    TEST_START();

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_sv39();
    rv_iommu_set_iohgatp_sv39x4();
    rv_iommu_set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    // Get virtual addresses
    uint64_t vaddr1 = virt_page_base(TWO_STAGE_W4K);
    uint64_t vaddr2 = TEST_VADDR_2MIB;
    uint64_t vaddr3 = TEST_VADDR_1GIB;
    uint64_t vaddr4 = virt_page_base(MSI_W2);

    uint64_t paddr1 = phys_page_base(TWO_STAGE_W4K);
    uint64_t paddr2 = TEST_PADDR_2MIB;
    uint64_t paddr3 = TEST_PADDR_1GIB;

    //# TEST 1: 4kiB translation
    // Setup translation
    rv_iommu_dbg_set_iova(vaddr1);
    rv_iommu_dbg_set_did(idma_ids[idma_idx]);
    rv_iommu_dbg_set_pv(false);
    rv_iommu_dbg_set_rw(true);
    rv_iommu_dbg_set_exe(false);
    rv_iommu_dbg_set_priv(true);

    // Trigger translation
    rv_iommu_dbg_set_go();

    // Wait for the transaction to be completed
    while (!rv_iommu_dbg_req_is_complete())
        ;
    
    // Check response register
    bool check = (!rv_iommu_dbg_req_fault() && !rv_iommu_dbg_req_is_superpage() &&
                    (rv_iommu_dbg_translated_ppn() == (paddr1 >> 12)));
    TEST_ASSERT("DBG IF: First transfer response matches", check);

    //# TEST 2: 2MiB translation
    // Setup translation
    rv_iommu_dbg_set_iova(vaddr2);
    rv_iommu_dbg_set_exe(true);
    rv_iommu_dbg_set_priv(false);

    // Trigger translation
    rv_iommu_dbg_set_go();

    // Wait for the transaction to be completed
    while (!rv_iommu_dbg_req_is_complete())
        ;
    
    // Check response register
    check = (!rv_iommu_dbg_req_fault() && rv_iommu_dbg_req_is_superpage() &&
                    ((rv_iommu_dbg_translated_ppn() & (~0x0FFULL)) == (paddr2 >> 12)));
    TEST_ASSERT("DBG IF: Second transfer response matches", check);

    // Check PPN encoding
    check = (rv_iommu_dbg_ppn_encode_x() == (uint8_t)8);
    TEST_ASSERT("DBG IF: PPN correctly encoded to 2MiB", check);

    //# TEST 3: 1GiB translation
    // Setup translation
    rv_iommu_dbg_set_iova(vaddr3);
    rv_iommu_dbg_set_exe(false);

    // Trigger translation
    rv_iommu_dbg_set_go();

    // Wait for the transaction to be completed
    while (!rv_iommu_dbg_req_is_complete())
        ;
    
    // Check response register
    check = (!rv_iommu_dbg_req_fault() && rv_iommu_dbg_req_is_superpage() &&
                    ((rv_iommu_dbg_translated_ppn() & (~0x1FFFFULL)) == (paddr3 >> 12)));
    TEST_ASSERT("DBG IF: Third transfer response matches", check);

    // Check PPN encoding
    check = (rv_iommu_dbg_ppn_encode_x() == (uint8_t)17);
    TEST_ASSERT("DBG IF: PPN correctly encoded to 2MiB", check);

    //# TEST 4: MSI translation using DBG IF (Error propagation)
    // Setup translation
    rv_iommu_dbg_set_iova(vaddr4);

    // Trigger translation
    rv_iommu_dbg_set_go();

    // Wait for the transaction to be completed
    while (!rv_iommu_dbg_req_is_complete())
        ;
    
    // Check response register
    check = (rv_iommu_dbg_req_fault());
    TEST_ASSERT("DBG IF: Fourth transfer generates fault, as it triggers MSI translation", check);

    // Check fault
    uint64_t fq_entry[4];
    if (rv_iommu_fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    bool check_cause = ((fq_entry[0] & CAUSE_MASK) == TRANS_TYPE_DISALLOWED);
    bool check_iova  = (fq_entry[2] == vaddr4);

    TEST_ASSERT("DBG IF w/ MSI: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("DBG IF w/ MSI: Recorded IOVA matches with input IOVA", check_iova);

    TEST_END();
}

/**
 *  Test to calculate latency using different number of PTs and devices
 */
bool latency_test(){

    TEST_START();

    /** Instantiate and map the DMA device */
    size_t idma_idx = 0;
    struct idma *dma_ut = (void*)idma_addr[idma_idx];

    uint64_t stamp_start = 0;
    uint64_t stamp_end = 0;
    uint64_t avg_lat = 0;
    uint64_t stamp = 0;

    fence_i();
    set_iommu_1lvl();
    rv_iommu_set_iosatp_sv39();
    rv_iommu_set_iohgatp_sv39x4();

    rv_iommu_ddt_inval(false, idma_ids[idma_idx]);
    rv_iommu_iotinval_vma(false, false, false, 0, 0, 0);
    rv_iommu_iotinval_gvma(false, false, 0, 0);

    rv_iommu_set_iohpmctr(0, 0);
    rv_iommu_set_iohpmctr(0, 1);
    rv_iommu_set_iohpmctr(0, 2);
    rv_iommu_set_iohpmctr(0, 3);
    rv_iommu_set_iohpmctr(0, 5);

    stamp = CSRR(CSR_CYCLES);
    srand(stamp);

    for (size_t i = 0; i < N_TRANSFERS; i++)
    {
        /** Instantiate and map the DMA device */
        size_t idma_idx = rand() % N_DMA;
        struct idma *dma_ut = (void*)idma_addr[idma_idx];

        size_t pt_index = rand() % N_MAPPINGS;

        //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
        uintptr_t read_paddr = phys_page_base(pt_index + STRESS_START);
        uintptr_t read_vaddr = virt_page_base(pt_index + STRESS_START);

        uintptr_t write_paddr = phys_page_base(pt_index + STRESS_START) + 0x0800;
        uintptr_t write_vaddr = virt_page_base(pt_index + STRESS_START) + 0x0800;

        fence_i();

        //# Write known values to memory
        write64(read_paddr, 0xDEADBEEF);
        write64(write_paddr, 0);
        
        idma_setup(dma_ut, (uint64_t)read_vaddr, (uint64_t)write_vaddr, 8);

        // Start stamp
        stamp_start = CSRR(CSR_CYCLES);

        // Check if iDMA was set up properly and init transfer
        uint64_t trans_id = read64((uintptr_t)&dma_ut->next_transfer_id);
        if (!trans_id)
            {ERROR("iDMA misconfigured")}

        // Poll transfer status
        while (read64((uintptr_t)&dma_ut->last_transfer_id_complete) != trans_id)
            ;

        // End stamp
        stamp_end = CSRR(CSR_CYCLES);

        fence_i();

        if (read64(write_paddr) != 0xDEADBEEF)
            {ERROR("Transfer does not match")}

        avg_lat += (stamp_end - stamp_start);
    }

    printf("Transfer average latency (in cycles): %llu\n", avg_lat/N_TRANSFERS);

    printf("Untranslated Requests cnt: %llu\n", rv_iommu_get_iohpmctr(0));
    printf("IOTLB miss cnt: %llu\n",            rv_iommu_get_iohpmctr(1));
    printf("DDT Walks cnt: %llu\n",             rv_iommu_get_iohpmctr(2));
    printf("First-stage PT walk cnt: %llu\n",   rv_iommu_get_iohpmctr(3));
    printf("Second-stage PT walk cnt: %llu\n",  rv_iommu_get_iohpmctr(4));

    TEST_END();
}

