#include <iommu_tests.h>
#include <command_queue.h>
#include <fault_queue.h>
#include <device_contexts.h>
#include <msi_pts.h>
#include <iommu_pts.h>
#include <hpm.h>
#include <rv_iommu.h>
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

/**
 *  iDMA device IDs 
 */
extern uint64_t idma_ids[];

/**
 *  iDMA Configuration Registers
 */
extern uintptr_t idma_src[];
extern uintptr_t idma_dest[];
extern uintptr_t idma_nbytes[];
extern uintptr_t idma_config[];
extern uintptr_t idma_status[];
extern uintptr_t idma_nextid[];
extern uintptr_t idma_done[];
extern uintptr_t idma_ipsr[];

/**
 *  IOMMU Memory-mapped registers 
 */
// fctl
extern uintptr_t fctl_addr;

// CQ
extern uintptr_t cqcsr_addr;
extern uintptr_t cqb_addr;
extern uintptr_t cqh_addr;
extern uintptr_t cqt_addr;

// FQ
extern uintptr_t fqb_addr;
extern uintptr_t fqh_addr;

// ipsr
extern uintptr_t ipsr_addr;

// icvec
extern uintptr_t icvec_addr;

// HPM
extern uintptr_t iocountovf_addr;
extern uintptr_t iocountihn_addr;
extern uintptr_t iohpmcycles_addr;
extern uintptr_t iohpmctr_addr[];
extern uintptr_t iohpmevt_addr[];

// MSI Cfg table
extern uintptr_t msi_addr_cq_addr;
extern uintptr_t msi_data_cq_addr;
extern uintptr_t msi_vec_ctl_cq_addr;
extern uintptr_t msi_addr_fq_addr;
extern uintptr_t msi_data_fq_addr;
extern uintptr_t msi_vec_ctl_fq_addr;
extern uintptr_t msi_addr_hpm_addr;
extern uintptr_t msi_data_hpm_addr;
extern uintptr_t msi_vec_ctl_hpm_addr;

// MSI Cfg table data
extern uint64_t msi_addr_cq;
extern uint64_t msi_addr_fq;
extern uint64_t msi_addr_hpm;
extern uint32_t msi_data_cq;
extern uint32_t msi_data_fq;
extern uint32_t msi_data_hpm;

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
    idma_setup(0, read_paddr1, write_paddr1, 8);

    // Check if iDMA was set up properly and init transfer
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check1 = (read64(write_paddr1) == 0x11);

    if (IDMA_IRQ_EN)
    {
        check1 = (read64(idma_ipsr[0]) == 3);
        TEST_ASSERT("iDMA interrupt pending bits set", check1);

        write64(idma_ipsr[0], (uint64_t) 0x03);

        check1 = (read64(idma_ipsr[0]) == 0);
        TEST_ASSERT("iDMA interrupt pending bits cleared after writing 1", check1);
    }

    /*------------- SECOND TRANSFER --------------*/
    idma_setup_addr(0, read_paddr2, write_paddr2);

    if (idma_exec_transfer(0) != 0)
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
    idma_setup(0, start_raddr1, start_waddr1, 24);

    // Check if iDMA was set up properly and init transfer
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    // Read from the physical addresses where the iDMA wrote. Compare with the initial values.
    bool check1, check2;
    check1 =   ((read64(start_waddr1     ) == 0x00) &&
                (read64(start_waddr1 + 8 ) == 0x10) &&
                (read64(start_waddr1 + 16) == 0x20));

    /*------------- SECOND TRANSFER --------------*/

    idma_setup_addr(0, start_raddr2, start_waddr2);

    // Check if iDMA was set up properly and init transfer
    if (idma_exec_transfer(0) != 0)
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

    //# Get addresses
    uintptr_t read_vaddr1 = virt_page_base(IOMMU_OFF_R);
    uintptr_t write_vaddr1 = virt_page_base(IOMMU_OFF_W);

    //# Config iDMA and init transfer
    idma_setup(0, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check fault record written in memory
    uint64_t fq_entry[4];
    if (fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    bool check_cause = ((fq_entry[0] & CAUSE_MASK) == ALL_INB_TRANSACTIONS_DISALLOWED);
    bool check_iova  = (fq_entry[2] == read_vaddr1);

    TEST_ASSERT("IOMMU Off: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("IOMMU Off: Recorded IOVA matches with input IOVA", check_iova);

    // Second entry
    if (fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    check_cause = ((fq_entry[0] & CAUSE_MASK) == ALL_INB_TRANSACTIONS_DISALLOWED);
    check_iova  = (fq_entry[2] == write_vaddr1);

    // Clear ipsr.fip
    write32(ipsr_addr, 0x7UL);

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

    //# Config iDMA and init transfer
    idma_setup(0, start_raddr1, start_waddr1, 24);
    if (idma_exec_transfer(0) != 0)
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

    fence_i();
    set_iommu_1lvl();
    set_iosatp_bare();
    set_iohgatp_bare();
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
    idma_setup(0, read_paddr1, write_paddr1, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Bare translation: First value matches", check);

    /*------- SECOND TRANSFER -------*/

    write64(idma_src[0], (uint64_t)read_paddr2);   // Source address
    write64(idma_dest[0], (uint64_t)write_paddr2); // Destination address

    //# Config iDMA and init transfer
    idma_setup_addr(0, read_paddr2, write_paddr2);
    if (idma_exec_transfer(0) != 0)
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

    fence_i();
    set_iommu_1lvl();
    set_iosatp_bare();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Bare | msiptp: Flat");

    //# DDTC Invalidation
    ddt_inval(false, idma_ids[0]);

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
    idma_setup(0, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Second-stage only: First transfer matches (Normal translation)", check);

    /*------- SECOND TRANSFER (if MSI translation is supported) -------*/
    if (MSI_TRANSLATION == 1)
    {
        //# Config iDMA and init transfer
        idma_setup_addr(0, read_vaddr2, write_vaddr2);
        if (idma_exec_transfer(0) != 0)
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

    fence_i();
    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
    VERBOSE("IOMMU 1LVL mode | iohgatp: Sv39x4 | iosatp: Sv39 | msiptp: Flat");

    //# DDTC Invalidation
    ddt_inval(false, idma_ids[0]);

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

    //# Config iDMA and init transfer
    idma_setup(0, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check first transfer
    bool check = (read64(write_paddr1) == 0x11);
    TEST_ASSERT("Two-stage translation: First transfer matches (4kiB pages)", check);

    /*------- SECOND TRANSFER -------*/

    //# Config iDMA and init transfer
    idma_setup_addr(0, read_vaddr2, write_vaddr2);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check second transfer
    check = (read64(write_paddr2) == 0x22);
    TEST_ASSERT("Two-stage translation: Second transfer matches (2MiB superpages)", check);

    /*------- THIRD TRANSFER -------*/

    //# Config iDMA and init transfer
    idma_setup_addr(0, read_vaddr3, write_vaddr3);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //# Check third transfer
    check = (read64(write_paddr3) == 0x33);
    TEST_ASSERT("Two-stage translation: Third transfer matches (1GiB superpages)", check);

    /*------- FOURTH TRANSFER (If MSI translation supported) -------*/

    if (MSI_TRANSLATION == 1)
    {
        //# Config iDMA and init transfer
        idma_setup_addr(0, read_vaddr4, write_vaddr4);
        if (idma_exec_transfer(0) != 0)
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
    idma_setup(0, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    //### SECOND TRANSFER
    idma_setup_addr(0, read_vaddr2, write_vaddr2);
    if (idma_exec_transfer(0) != 0)
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
    iotinval_vma(false, true, true, 0, 0xABC, 0xDEF);

    //### Perform previous transfers again
    idma_setup_addr(0, read_vaddr1, write_vaddr1);  // first
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    idma_setup_addr(0, read_vaddr2, write_vaddr2);  // second
    if (idma_exec_transfer(0) != 0)
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
    iotinval_gvma(false, true, 0, 0xABC);

    //### Perform previous transfers again
    idma_setup_addr(0, read_vaddr1, write_vaddr1);  // first
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

    idma_setup_addr(0, read_vaddr2, write_vaddr2);  // second
    if (idma_exec_transfer(0) != 0)
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

    fence_i();
    set_iommu_1lvl();
    set_iosatp_sv39();
    set_iohgatp_sv39x4();
    set_msi_flat();
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

    idma_setup(0, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

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
    // Read FQ record by DWs
    uint64_t fq_entry[4];
    if (fq_read_record(fq_entry) != 0)
        {ERROR("IOMMU did not generated a new FQ record when expected")}

    bool check_cause = ((fq_entry[0] & CAUSE_MASK) == LOAD_PAGE_FAULT);
    bool check_iova  = (fq_entry[2] == read_vaddr1);

    TEST_ASSERT("Read 1: Cause code matches with induced fault code", check_cause);
    TEST_ASSERT("Read 1: Recorded IOVA matches with input IOVA", check_iova);

    // Second entry
    if (fq_read_record(fq_entry) != 0)
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

    uint32_t cqh = read32(cqh_addr);

    //# Send command
    iofence_c(true, true, IOFENCE_ADDR, IOFENCE_DATA);

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
    check = (iofence_data == IOFENCE_DATA);
    TEST_ASSERT("IOFENCE DATA was correctly written in the given ADDR", check);

    // Clear cqcsr.fence_w_ip and ipsr
    write32(cqcsr_addr, cqcsr);
    write32(ipsr_addr, 0x7UL);

    TEST_END();
}

/**
 *  Induce a fault in the IOMMU with a misconfigured translation.
 *  Also induce a fault in the CQ with a misconfigured command.
 */
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
    set_ig_msi();

    //# Induce a fault in the FQ with a misconfigured translation
    uintptr_t read_vaddr1 = virt_page_base(MSI_GEN_R);
    uintptr_t write_vaddr1 = virt_page_base(MSI_GEN_W);

    idma_setup(0, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}


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
    write32(msi_vec_ctl_cq_addr, 0x0UL);

    // Flush cache
    fence_i();

    // Check data for CQ vector after clearing mask
    uint32_t cq_msi_data = read32((uintptr_t)msi_addr_cq);
    check = (cq_msi_data == msi_data_cq);
    TEST_ASSERT("MSI data corresponding to CQ interrupt vector matches after clearing mask", check);

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
    set_ig_msi();

    // Disable clock counter
    iocountinh = read32(iocountihn_addr) | 0x1UL;
    write32(iocountihn_addr, iocountinh);

    // Set iohpmcycles initial value
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
    set_ig_wsi();

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

    idma_setup(0, read_vaddr1, write_vaddr1, 8);
    if (idma_exec_transfer(0) != 0)
        {ERROR("iDMA misconfigured")}

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

/**
 *  Test to calculate latency using different number of PTs and devices
 */
bool latency_test(){

    TEST_START();

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
        size_t read_index = rand() % N_RD_MAPPINGS;
        size_t write_index = rand() % N_WR_MAPPINGS;

        //# Get a set of Guest-Virtual-to-Supervisor-Physical mappings
        uintptr_t read_paddr = phys_page_base(read_index + STRESS_START_RD);
        uintptr_t read_vaddr = virt_page_base(read_index + STRESS_START_RD);

        uintptr_t write_paddr = phys_page_base(write_index + STRESS_START_WR);
        uintptr_t write_vaddr = virt_page_base(write_index + STRESS_START_WR);

        fence_i();

        //# Write known values to memory
        write64(read_paddr, 0xDEADBEEF);
        write64(write_paddr, 0);

        write64(idma_src[dev_index], (uint64_t)read_vaddr);     // Source address
        write64(idma_dest[dev_index], (uint64_t)write_vaddr);   // Destination address
        write64(idma_nbytes[dev_index], 8);                     // N of bytes to be transferred
        write64(idma_config[dev_index], 0);                     // iDMA config: Disable decouple, deburst and serialize

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

