/**
 * !NOTES:
 * 
 *  -   
 */

#include <command_queue.h>

uintptr_t cqcsr_addr = IOMMU_REG_ADDR(IOMMU_CQCSR_OFFSET);
uintptr_t cqb_addr = IOMMU_REG_ADDR(IOMMU_CQB_OFFSET);
uintptr_t cqh_addr = IOMMU_REG_ADDR(IOMMU_CQH_OFFSET);
uintptr_t cqt_addr = IOMMU_REG_ADDR(IOMMU_CQT_OFFSET);

// N_entries * 16 bytes
uint64_t command_queue[CQ_N_ENTRIES * 2 * sizeof(uint64_t)] __attribute__((aligned(PAGE_SIZE)));

void cq_init()
{
    uint64_t    cqb;
    uint32_t    cqh;

    // Configure cqb with base PPN of the queue and size as log2(N)
    cqb = ((((uintptr_t)command_queue) >> 2) & CQB_PPN_MASK) | CQ_LOG2SZ_1;
    write64(cqb_addr, cqb);

    // Set cqt equal to cqh
    cqh = read32(cqh_addr);
    write32(cqt_addr, cqh);

    // Write 1 to cqcsr.cqen to enable the CQ
    write32(cqcsr_addr, CQCSR_CQEN | CQCSR_CIE);

    // Poll cqcsr.cqon until it reads 1
    while (!(read32(cqcsr_addr) & CQCSR_CQON));
}

void ddt_inval(bool dv, uint64_t device_id)
{
    // Construct command
    uint64_t cmd_entry[2];

    INFO("Writing IODIR.INVAL_DDT to CQ")
    cmd_entry[0]    = IODIR | INVAL_DDT;

    // Add device_id if needed
    if (dv)
        cmd_entry[0]    |= IODIR_DV | (device_id << IODIR_DID_OFF);

    cmd_entry[1]    = 0;

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cq_entry_base = ((uintptr_t)command_queue & CQ_PPN_MASK) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);
}

void iotinval_vma(bool av, bool gv, bool pscv, uint64_t addr, uint64_t gscid, uint64_t pscid)
{
    // Construct command
    uint64_t cmd_entry[2];

    INFO("Writing IOTINVAL.VMA to CQ")
    cmd_entry[0]    = IOTINVAL | VMA;

    // Add GSCID
    if (gv)
        cmd_entry[0] |= (IOTINVAL_GV | (gscid << IOTINVAL_GSCID_OFF));

    // Add PSCID
    if (pscv)
        cmd_entry[0] |= (IOTINVAL_PSCV | (pscid << IOTINVAL_PSCID_OFF));

    cmd_entry[1]    = 0;

    // Add ADDR
    if (av)
        cmd_entry[1] |= (IOTINVAL_AV | ((addr >> 12) << IOTINVAL_IOVA_OFF));

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cq_entry_base = ((uintptr_t)command_queue & CQ_PPN_MASK) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);
}

void iotinval_gvma(bool av, bool gv, uint64_t addr, uint64_t gscid)
{
    // Construct command
    uint64_t cmd_entry[2];

    INFO("Writing IOTINVAL.GVMA to CQ")
    cmd_entry[0]    = IOTINVAL | GVMA;

    // Add GSCID
    if (gv)
        cmd_entry[0] |= (IOTINVAL_GV | (gscid << IOTINVAL_GSCID_OFF));

    cmd_entry[1]    = 0;

    // Add ADDR
    if (av)
        cmd_entry[1] |= (IOTINVAL_AV | ((addr >> 12) << IOTINVAL_IOVA_OFF));

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cq_entry_base = ((uintptr_t)command_queue & CQ_PPN_MASK) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);
}

void iofence_c(bool wsi, bool av, uint64_t addr, uint64_t data)
{
    uint64_t cmd_entry[2];

    INFO("Writing IOFENCE to CQ")
    cmd_entry[0]    = IOFENCE | FUNC3_C;

    // Add WSI
    if (wsi)
        cmd_entry[0]    |= IOFENCE_WSI;

    // Add AV
    if (av)
    {
        cmd_entry[0]    |= IOFENCE_AV;
        cmd_entry[0]    |= (data << 32);
        cmd_entry[1]    = (addr >> 2);
    }

    // Read cqt
    uint64_t cqt = read32(cqt_addr);

    // Get address of the next entry to write in the CQ
    uintptr_t cq_entry_base = ((uintptr_t)command_queue & CQ_PPN_MASK) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, cmd_entry[0]);
    write64(cq_entry_base + 8, cmd_entry[1]);

    // Increment tail reg
    cqt++;
    write32(cqt_addr, cqt);
}

