#include <rv_iommu.h>
#include <iommu_tests.h>
#include <page_tables.h>

#define TR_REQ_CTL_DID_OFFSET   40
#define TR_REQ_CTL_DID_MASK     0xFFFFFF0000000000ULL
#define TR_REQ_CTL_PID_OFFSET   12
#define TR_REQ_CTL_PID_MASK     0xFFFFF000ULL
#define TR_REQ_CTL_PV_BIT       (1ULL << 32)

#define TR_REQ_CTL_GO_BIT       (1ULL << 0)
#define TR_REQ_CTL_PRIV_BIT     (1ULL << 1)
#define TR_REQ_CTL_EXE_BIT      (1ULL << 2)
#define TR_REQ_CTL_NW_BIT       (1ULL << 3)

#define TR_RESPONSE_FAULT_BIT   (1ULL << 0)
#define TR_RESPONSE_SP_BIT      (1ULL << 9)
#define TR_RESPONSE_PPN_OFFSET  (10)
#define TR_RESPONSE_PPN_MASK    (0x3FFFFFFFFFFC00ULL)

#define IOMMU_MAX_MSI_CFG_TABLE 16

typedef struct debug {
  uint64_t tr_req_iova; // translation-request IOVA
  uint64_t tr_req_ctl;  // translation-request control
  uint64_t tr_response; // translation-request response
} iommu_debug_t;

typedef struct msi_cfg_table_t {
  uint64_t addr;
  uint32_t data;
  uint32_t vctl;
} iommu_msi_cfg_table_t;

typedef struct iommu {
  uint64_t capabilities;// IOMMU implemented capabilities
  uint32_t fctl;        // features control
  uint32_t reserved0;
  uint64_t ddtp;        // device directory table pointer
  uint64_t cqb;         // command-queue base
  uint32_t cqh;         // command-queue head
  uint32_t cqt;         // command-queue tail
  uint64_t fqb;         // fault-queue base
  uint32_t fqh;         // fault-queue head
  uint32_t fqt;         // fault-queue tail
  uint64_t pqb;         // page-request-queue base
  uint32_t pqh;         // page-request-queue head
  uint32_t pqt;         // page-request-queue tail
  uint32_t cqcsr;       // command-queue csr
  uint32_t fqcsr;       // fault-queue csr
  uint32_t pqcsr;       // page-request-queue csr
  uint32_t ipsr;        // interrupt pending status register
  uint32_t iocntovf;    // hpm counter overflows
  uint32_t iocntinh;    // hpm counter inhibits
  uint64_t iohpmcycles; // hpm cycles counter
  uint64_t iohpmctr[IOMMU_MAX_HPM_COUNTERS]; // hpm event counters
  uint64_t iohpmevt[IOMMU_MAX_HPM_COUNTERS]; // hpm event selector
  iommu_debug_t debug_inf; // IOMMU debug interface
  uint64_t reserved[8]; // reserved for future use
  uint64_t custom[9];   // designated for costum use
  uint64_t icvec;       // interrupt cause to interrupt vector
  iommu_msi_cfg_table_t msi_cfg_tbl[IOMMU_MAX_MSI_CFG_TABLE]; 
}__attribute__((__packed__, aligned(PAGE_SIZE))) iommu_t;

typedef uint64_t command_t[2];

/** IOMMU hw structure only visible inside IOMMU driver */
static iommu_t *iommu = (void*)IOMMU_BASE_ADDR;

// N_entries * 16 bytes
uint64_t command_queue[CQ_N_ENTRIES * 2 * sizeof(uint64_t)] __attribute__((aligned(PAGE_SIZE)));

// N_entries * 32 bytes
uint64_t fault_queue[FQ_N_ENTRIES * 4 * sizeof(uint64_t)] __attribute__((aligned(PAGE_SIZE)));

// DDT
ddt_t root_ddt[DDT_N_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

// First and second-stage page tables (Already configured)
extern pte_t s1pt[][512];
extern pte_t s2pt_root[];
// MSI page tables (Configured in msi_pts.c)
extern uint64_t msi_pt[];

/*******************************************************************************************************
*                                   MSI Config Table Related Functions                                 *
*******************************************************************************************************/
static inline void rv_iommu_set_msi_cfg_tbl_addr(size_t msi_tlb_entry, uint64_t new_addr)
{
    write64((uintptr_t)&iommu->msi_cfg_tbl[msi_tlb_entry].addr, new_addr);
}

uint64_t rv_iommu_get_msi_cfg_tbl_addr(size_t msi_tlb_entry)
{
    read64((uintptr_t)&iommu->msi_cfg_tbl[msi_tlb_entry].addr);
}

static inline void rv_iommu_set_msi_cfg_tbl_data(size_t msi_tlb_entry, uint32_t new_data)
{
    write32((uintptr_t)&iommu->msi_cfg_tbl[msi_tlb_entry].data, new_data);
}

void rv_iommu_set_msi_cfg_tbl_vctl(size_t msi_tlb_entry, uint32_t new_vctl)
{
    write32((uintptr_t)&iommu->msi_cfg_tbl[msi_tlb_entry].vctl, new_vctl);
}

/*******************************************************************************************************
*                                   Command-Queue Related Functions                                    *
*******************************************************************************************************/

static void rv_iommu_ddt_init(void)
{
    // Init all entries to zero
    for (int i = 0; i < DDT_N_ENTRIES; i++)
    {
        root_ddt[i].tc = 0;
    }

    // Configure DCs in the root DDT (1LVL mode)
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].tc = test_dc_tc_table[BASIC];
        root_ddt[i].iohgatp = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_BARE);
        root_ddt[i].iohgatp |= (GSCID_ARRAY[i] << GSCID_OFF);
        root_ddt[i].ta = (PSCID_ARRAY[i] << PSCID_OFF);
        root_ddt[i].fsc = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_BARE);

        if (MSI_TRANSLATION == 1)
        {
            root_ddt[i].msiptp = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_OFF);
            root_ddt[i].msi_addr_mask = MSI_ADDR_MASK;
            root_ddt[i].msi_addr_pattern = MSI_ADDR_PATTERN;
            root_ddt[i].reserved = 0;
        }
    }
}

void rv_iommu_set_iosatp_bare(void)
{
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].ta = (PSCID_ARRAY[i] << PSCID_OFF);
        root_ddt[i].fsc = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_BARE);
    }
}

void rv_iommu_set_iosatp_sv39()
{
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].ta = (PSCID_ARRAY[i] << PSCID_OFF);
        root_ddt[i].fsc = (((uintptr_t)s1pt) >> 12) | (IOSATP_MODE_SV39);
    }
}

void rv_iommu_set_iohgatp_bare()
{
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].iohgatp = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_BARE);
        root_ddt[i].iohgatp |= (GSCID_ARRAY[i] << GSCID_OFF);
    }
}

void rv_iommu_set_iohgatp_sv39x4()
{
    for (int i = DID_MIN; i < DID_MAX + 1; i++)
    {
        root_ddt[i].iohgatp = (((uintptr_t)s2pt_root) >> 12) | (IOHGATP_MODE_SV39X4);
        root_ddt[i].iohgatp |= (GSCID_ARRAY[i] << GSCID_OFF);
    }
}

void rv_iommu_set_msi_off()
{
    if (MSI_TRANSLATION == 1)
    {
        for (int i = DID_MIN; i < DID_MAX + 1; i++)
        {
            root_ddt[i].msiptp = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_OFF);
            root_ddt[i].msi_addr_mask = MSI_ADDR_MASK;
            root_ddt[i].msi_addr_pattern = MSI_ADDR_PATTERN;
        }
    }
}

void rv_iommu_set_msi_flat()
{
    if (MSI_TRANSLATION == 1)
    {
        for (int i = DID_MIN; i < DID_MAX + 1; i++)
        {
            root_ddt[i].msiptp = (((uintptr_t)msi_pt) >> 12) | (MSIPTP_MODE_FLAT);
            root_ddt[i].msi_addr_pattern = MSI_ADDR_MASK;
            root_ddt[i].msi_addr_pattern = MSI_ADDR_PATTERN;
        }
    }
}

/*******************************************************************************************************
*******************************************************************************************************/

/*******************************************************************************************************
*                                   Command-Queue Related Functions                                    *
*******************************************************************************************************/
static inline void rv_iommu_write_command_in_queue(command_t new_cmd)
{
    uint32_t cqt = read32((uintptr_t)&iommu->cqt);
    // Get address of the next entry to write in the CQ
    uintptr_t cq_entry_base = ((uintptr_t)command_queue & CQ_PPN_MASK) | (cqt << 4);

    // Write command to memory
    write64(cq_entry_base, new_cmd[0]);
    write64(cq_entry_base + 8, new_cmd[1]);

    // increment the command-queue tail index
    cqt++;
    write32((uintptr_t)&iommu->cqt, cqt);
}

void rv_iommu_cq_init(void)
{
    // Configure cqb with base PPN of the queue and size as log2(N)
    write64((uintptr_t)&iommu->cqb, ((((uintptr_t)command_queue) >> 2) & CQB_PPN_MASK) | CQ_LOG2SZ_1);

    // Set cqt equal to cqh
    write32((uintptr_t)&iommu->cqt, read32((uintptr_t)&iommu->cqh));

    // Write 1 to cqcsr.cqen to enable the CQ
    write32((uintptr_t)&iommu->cqcsr, CQCSR_CQEN | CQCSR_CIE);

    // Poll cqcsr.cqon until it reads 1
    while (!(read32((uintptr_t)&iommu->cqcsr) & CQCSR_CQON));
}

uint32_t rv_iommu_get_cqh(void)
{
    return read32((uintptr_t)&iommu->cqh);
}

uint32_t rv_iommu_get_cqcsr(void)
{
    return read32((uintptr_t)&iommu->cqcsr);
}

void rv_iommu_set_cqcsr(uint32_t new_cqcsr)
{
    write32((uintptr_t)&iommu->cqcsr, new_cqcsr);
}

void rv_iommu_induce_fault_cq(void)
{
    //# Induce a fault in the CQ
    command_t new_cmd;
    new_cmd[0]    = IOTINVAL | GVMA;

    // Add PSCID (invalid for IOTINVAL.GVMA)
    new_cmd[0]    |= IOTINVAL_PSCV;

    new_cmd[1]    = 0;

    rv_iommu_write_command_in_queue(new_cmd);
}

void rv_iommu_ddt_inval(bool dv, uint64_t device_id)
{
    command_t new_cmd;

    INFO("Writing IODIR.INVAL_DDT to CQ")
    new_cmd[0]    = IODIR | INVAL_DDT;
    // Add device_id if needed
    if (dv){
        new_cmd[0]    |= IODIR_DV | (device_id << IODIR_DID_OFF);
    }
    new_cmd[1]    = 0;

    rv_iommu_write_command_in_queue(new_cmd);
}

void rv_iommu_iotinval_vma(bool av, bool gv, bool pscv, uint64_t addr, uint64_t gscid, uint64_t pscid)
{
    command_t new_cmd;

    INFO("Writing IOTINVAL.VMA to CQ")
    new_cmd[0]    = IOTINVAL | VMA;

    // Add GSCID
    if (gv)
        new_cmd[0] |= (IOTINVAL_GV | (gscid << IOTINVAL_GSCID_OFF));

    // Add PSCID
    if (pscv)
        new_cmd[0] |= (IOTINVAL_PSCV | (pscid << IOTINVAL_PSCID_OFF));

    new_cmd[1]    = 0;

    // Add ADDR
    if (av)
        new_cmd[1] |= (IOTINVAL_AV | ((addr >> 12) << IOTINVAL_IOVA_OFF));

   rv_iommu_write_command_in_queue(new_cmd);
}

void rv_iommu_iotinval_gvma(bool av, bool gv, uint64_t addr, uint64_t gscid)
{
    command_t new_cmd;

    INFO("Writing IOTINVAL.GVMA to CQ")
    new_cmd[0]    = IOTINVAL | GVMA;

    // Add GSCID
    if (gv)
        new_cmd[0] |= (IOTINVAL_GV | (gscid << IOTINVAL_GSCID_OFF));

    new_cmd[1]    = 0;

    // Add ADDR
    if (av)
        new_cmd[1] |= (IOTINVAL_AV | ((addr >> 12) << IOTINVAL_IOVA_OFF));

    rv_iommu_write_command_in_queue(new_cmd);
}

void rv_iommu_iofence_c(bool wsi, bool av)
{
    command_t new_cmd;

    INFO("Writing IOFENCE to CQ")
    new_cmd[0]    = IOFENCE | FUNC3_C;

    // Add WSI
    if (wsi)
        new_cmd[0]    |= IOFENCE_WSI;

    // Add AV
    if (av)
    {
        new_cmd[0]    |= IOFENCE_AV;
        new_cmd[0]    |= (IOFENCE_DATA << 32);
        new_cmd[1]    = (IOFENCE_ADDR >> 2);
    }

    rv_iommu_write_command_in_queue(new_cmd);
}

uint32_t rv_iommu_get_iofence(void)
{
    uintptr_t iofence_addr = IOFENCE_ADDR;
    uint32_t iofence_data = read32(iofence_addr);
    return iofence_data;
}

static void rv_iommu_fq_init(void)
{
    uint64_t   fqb;

    // Configure fqb with base PPN of the queue and size as log2(N)
    fqb = ((((uintptr_t)fault_queue) >> 2) & FQB_PPN_MASK) | FQ_LOG2SZ_1;
    write64((uintptr_t)&iommu->fqb, fqb);

    write32((uintptr_t)&iommu->fqh, read32((uintptr_t)&iommu->fqt));

    // Write 1 to fqcsr.fqen to enable the FQ
    write32((uintptr_t)&iommu->fqcsr, FQCSR_FQEN | FQCSR_FIE);

    // Poll fqcsr.fqon until it reads 1
    while (!(read32((uintptr_t)&iommu->fqcsr) & FQCSR_FQON));
}
/*******************************************************************************************************
*******************************************************************************************************/

int rv_iommu_fq_read_record(uint64_t *buf)
{
    // Read fqh
    uint32_t fqh = read32((uintptr_t)&iommu->fqh);

    // Read fqt
    uint32_t fqt = read32((uintptr_t)&iommu->fqt);

    if (fqh != fqt) {
        // Get address of the next entry in the FQ
        uintptr_t fq_entry_base = ((uintptr_t)fault_queue & FQ_PPN_MASK) | (fqh << 5);

        // Read FQ record by DWs
        buf[0] = read64(fq_entry_base + 0 );
        buf[1] = read64(fq_entry_base + 8 );
        buf[2] = read64(fq_entry_base + 16);
        buf[3] = read64(fq_entry_base + 24);

        fqh++;
        write32((uintptr_t)&iommu->fqh, fqh);

        return 0;
    } else {
        return -1;
    }
}

/**
 *  Set IOMMU OFF. All transactions are disallowed and blocked
 */
void set_iommu_off()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_OFF);

    write64((uintptr_t)&iommu->ddtp, ddtp);
}

void set_iommu_bare()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_BARE);

    write64((uintptr_t)&iommu->ddtp, ddtp);
}

void set_iommu_1lvl()
{
    // Program ddtp register with DDT mode and root DDT base PPN
    uintptr_t ddtp = ((((uintptr_t)root_ddt) >> 2) & DDTP_PPN_MASK) | (DDTP_MODE_1LVL);
    
    write64((uintptr_t)&iommu->ddtp, ddtp);
}

void set_ig_wsi()
{
    uint32_t fctl = (1UL << 1);
    write32((uintptr_t)&iommu->fctl, fctl);
}

void set_ig_msi()
{
    uint32_t fctl = (0UL << 1);
    write32((uintptr_t)&iommu->fctl, fctl);
}

uint32_t rv_iommu_get_ipsr() 
{
    return read32((uintptr_t)&iommu->ipsr);
}

void rv_iommu_set_ipsr(uint32_t ipsr_new) 
{
    write32((uintptr_t)&iommu->ipsr, ipsr_new);
}

void rv_iommu_clear_ipsr_fip()
{
    rv_iommu_set_ipsr(0x7UL);
}

uint32_t rv_iommu_get_iocountovf()
{
    return read32((uintptr_t)&iommu->iocntovf);
}

uint32_t rv_iommu_get_iocountihn()
{
    return read32((uintptr_t)&iommu->iocntinh);
}

void rv_iommu_set_iocountihn(uint32_t iocountihn_new)
{
    return write32((uintptr_t)&iommu->iocntinh, iocountihn_new);
}

uint64_t rv_iommu_get_iohpmcycles()
{
    return read64((uintptr_t)&iommu->iohpmcycles);
}

void rv_iommu_set_iohpmcycles(uint64_t iohpmcycles_new)
{
    return write64((uintptr_t)&iommu->iohpmcycles, iohpmcycles_new);
}

uint64_t rv_iommu_get_iohpmctr (size_t counter_idx)
{
    return read64((uintptr_t)&iommu->iohpmctr[counter_idx]);
}

void rv_iommu_set_iohpmctr(uint64_t iohpmctr_new, size_t counter_idx)
{
    return write64((uintptr_t)&iommu->iohpmctr[counter_idx], iohpmctr_new);
}

uint64_t rv_iommu_get_iohpmevt (size_t counter_idx)
{
    return read64((uintptr_t)&iommu->iohpmevt[counter_idx]);
}

void rv_iommu_set_iohpmevt(uint64_t iohpmevt_new, size_t counter_idx)
{
    return write64((uintptr_t)&iommu->iohpmevt[counter_idx], iohpmevt_new);
}

void rv_iommu_set_icvec(uint64_t icvec_new)
{
    return write64((uintptr_t)&iommu->icvec, icvec_new);
}

void rv_iommu_dbg_set_iova(uint64_t iova)
{
    write64((uintptr_t)&iommu->debug_inf.tr_req_iova, (iova & ~0xFFFULL));
}

static inline uint64_t rv_iommu_dbg_get_ctl()
{
    return read64((uintptr_t)&iommu->debug_inf.tr_req_ctl);
}

static inline uint64_t rv_iommu_dbg_get_response()
{
    return read64((uintptr_t)&iommu->debug_inf.tr_response);
}

void rv_iommu_dbg_set_did(uint64_t device_id)
{
    uint64_t ctl_tmp = rv_iommu_dbg_get_ctl();
    ctl_tmp |= ((device_id << TR_REQ_CTL_DID_OFFSET) & TR_REQ_CTL_DID_MASK);

    write64((uintptr_t)&iommu->debug_inf.tr_req_ctl, ctl_tmp);
};

void rv_iommu_dbg_set_pv(bool pv)
{
    uint64_t ctl_tmp = rv_iommu_dbg_get_ctl();
    if (pv){
        ctl_tmp |= TR_REQ_CTL_PV_BIT;
    } else {
        ctl_tmp &= (~TR_REQ_CTL_PV_BIT);
    }

    write64((uintptr_t)&iommu->debug_inf.tr_req_ctl, ctl_tmp);
};

void rv_iommu_dbg_set_priv(bool priv)
{
    uint64_t ctl_tmp = rv_iommu_dbg_get_ctl();
    if (priv){
        ctl_tmp |= TR_REQ_CTL_PRIV_BIT;
    } else {
        ctl_tmp &= (~TR_REQ_CTL_PRIV_BIT);
    }

    write64((uintptr_t)&iommu->debug_inf.tr_req_ctl, ctl_tmp);
};

void rv_iommu_dbg_set_rw(bool rw)
{
    uint64_t ctl_tmp = rv_iommu_dbg_get_ctl();
    if (rw){
        ctl_tmp &= (~TR_REQ_CTL_NW_BIT);
    } else {
        ctl_tmp |= TR_REQ_CTL_NW_BIT;
    }

    write64((uintptr_t)&iommu->debug_inf.tr_req_ctl, ctl_tmp);
};

void rv_iommu_dbg_set_exe(bool exe)
{
    uint64_t ctl_tmp = rv_iommu_dbg_get_ctl();
    if (exe){
        ctl_tmp |= TR_REQ_CTL_EXE_BIT;
    } else {
        ctl_tmp &= (~TR_REQ_CTL_EXE_BIT);
    }

    write64((uintptr_t)&iommu->debug_inf.tr_req_ctl, ctl_tmp);
};

void rv_iommu_dbg_set_go(void)
{
    uint64_t ctl_tmp = rv_iommu_dbg_get_ctl();
    ctl_tmp |= TR_REQ_CTL_GO_BIT;
    write64((uintptr_t)&iommu->debug_inf.tr_req_ctl, ctl_tmp);
}

bool rv_iommu_dbg_req_is_complete(void)
{
    uint64_t ctl_tmp = rv_iommu_dbg_get_ctl();
    
    return ((ctl_tmp & TR_REQ_CTL_GO_BIT != 0) ? (false) : (true));
}

uint8_t rv_iommu_dbg_req_fault(void)
{
    uint64_t resp_tmp = rv_iommu_dbg_get_response();
    return (uint8_t)(resp_tmp & TR_RESPONSE_FAULT_BIT);
}

uint8_t rv_iommu_dbg_req_is_superpage(void)
{
    uint64_t resp_tmp = rv_iommu_dbg_get_response();
    return (uint8_t)((resp_tmp & TR_RESPONSE_SP_BIT) >> 9);
}

uint64_t rv_iommu_dbg_translated_ppn(void)
{
    uint64_t resp_tmp = rv_iommu_dbg_get_response();
    return ((resp_tmp & TR_RESPONSE_PPN_MASK) >> TR_RESPONSE_PPN_OFFSET);
}

uint8_t rv_iommu_dbg_ppn_encode_x(void)
{
    uint64_t resp_tmp = rv_iommu_dbg_get_response();
    uint8_t bit = 0, x_idx = 0;

    uint64_t ppn = (resp_tmp & TR_RESPONSE_PPN_MASK) >> TR_RESPONSE_PPN_OFFSET;
    while ((bit = (uint8_t)(ppn & 0x01ULL)) != 0)
    {
        ppn = (ppn >> 1);
        x_idx++;
    }
    
    return x_idx;
}

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
    rv_iommu_cq_init();
    VERBOSE("CQ: Interrupts enabled");

    //# Setup the Fault Queue:
    // Allocate a buffer of N (POT) entries (32-bytes each).
    // This buffer must be alligned to the greater of two values: 4-kiB or N x 32 bytes.
    // Configure fqb with the queue size as log2(N) and the base address of the buffer.
    // Set fqh to zero.
    // Enable the FQ by writing 1 to fqcsr.fqen, poll fqcsr.fqon until it reads 1.
    INFO("Configuring FQ");
    rv_iommu_fq_init();
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
    rv_iommu_ddt_init();
    set_iommu_off();

    //# Configure MSI Config Table
    INFO("Configuring IOMMU interrupts: MSI config table");
    // CQ
    INFO("Configuring IOMMU CQ interrupt");
    rv_iommu_set_msi_cfg_tbl_addr(CQ_INT_VECTOR, MSI_ADDR_CQ);
    rv_iommu_set_msi_cfg_tbl_data(CQ_INT_VECTOR, MSI_DATA_CQ);
    rv_iommu_set_msi_cfg_tbl_vctl(CQ_INT_VECTOR, MSI_VCTL_CQ);

    // FQ
    INFO("Configuring IOMMU FQ interrupt");
    rv_iommu_set_msi_cfg_tbl_addr(FQ_INT_VECTOR, MSI_ADDR_FQ);
    rv_iommu_set_msi_cfg_tbl_data(FQ_INT_VECTOR, MSI_DATA_FQ);
    rv_iommu_set_msi_cfg_tbl_vctl(FQ_INT_VECTOR, MSI_VCTL_FQ);

    // HPM
    INFO("Configuring IOMMU HPM interrupt");
    rv_iommu_set_msi_cfg_tbl_addr(HPM_INT_VECTOR, MSI_ADDR_HPM);
    rv_iommu_set_msi_cfg_tbl_data(HPM_INT_VECTOR, MSI_DATA_HPM);
    rv_iommu_set_msi_cfg_tbl_vctl(HPM_INT_VECTOR, MSI_VCTL_HPM);

    INFO("Configuring IGS to WSI");
    //# Configure the IOMMU to generate interrupts as WSI by default
    set_ig_wsi();

    // //# Configure the IOMMU to generate interrupts as MSI
    // set_ig_msi();

    //# Setup icvec register with an interrupt vector for each cause
    INFO("Setting up interrupt vectors");
    uint64_t icvec = (HPM_INT_VECTOR << 8) | (FQ_INT_VECTOR << 4) | (CQ_INT_VECTOR << 0);
    rv_iommu_set_icvec(icvec);

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

    rv_iommu_set_iohpmevt(iohpmevt[0], 0);
    rv_iommu_set_iohpmevt(iohpmevt[1], 1);
    rv_iommu_set_iohpmevt(iohpmevt[2], 2);
    rv_iommu_set_iohpmevt(iohpmevt[3], 3);
    rv_iommu_set_iohpmevt(iohpmevt[4], 4);

    // Enable counters by writing to iocountinh
    uint32_t iocountinh = (uint32_t)(~(CNT_MASK));    // Enable counters
    rv_iommu_set_iocountihn(iocountinh);

    VERBOSE("IOMMU off | iohgatp: Bare | iosatp: Bare | msiptp: Flat");
}