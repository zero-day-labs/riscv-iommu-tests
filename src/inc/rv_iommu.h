#ifndef _RV_IOMMU_H_
#define _RV_IOMMU_H_

#include <iommu_tests.h>

#define IOMMU_MAX_HPM_COUNTERS 31

// Number of entries in the CQ. Must be POT
#define CQ_N_ENTRIES    (64 )
// Size of the queue represented as Log2(64)-1 = 5
#define CQ_LOG2SZ_1     (5  )

// Mask for ddtp.PPN (ddtp[53:10])
#define DDTP_PPN_MASK    (0x3FFFFFFFFFFC00ULL)

// Define DDT (IOMMU) mode
#define DDTP_MODE_OFF   (0ULL)
#define DDTP_MODE_BARE  (1ULL)
#define DDTP_MODE_1LVL  (2ULL)
#define DDTP_MODE_2LVL  (3ULL)
#define DDTP_MODE_3LVL  (4ULL)

// IOMMU Register Offsets
#define IOMMU_CAPABILITIES_OFFSET   0x0
#define IOMMU_FCTL_OFFSET           0x8
#define IOMMU_DDTP_OFFSET           0x10
#define IOMMU_CQB_OFFSET            0x18
#define IOMMU_CQH_OFFSET            0x20
#define IOMMU_CQT_OFFSET            0x24
#define IOMMU_FQB_OFFSET            0x28
#define IOMMU_FQH_OFFSET            0x30
#define IOMMU_FQT_OFFSET            0x34
#define IOMMU_CQCSR_OFFSET          0x48
#define IOMMU_FQCSR_OFFSET          0x4c
#define IOMMU_IPSR_OFFSET           0x54
#define IOMMU_IOCOUNTOVF_OFFSET     0x58
#define IOMMU_IOCOUNTINH_OFFSET     0x5c
#define IOMMU_IOHPMCYCLES_OFFSET    0x60
#define IOMMU_IOHPMCTR_OFFSET       0x68
#define IOMMU_IOHPMEVT_OFFSET       0x160
#define IOMMU_TR_REQ_IOVA_OFFSET    0x258
#define IOMMU_TR_REQ_CTL_OFFSET     0x260
#define IOMMU_TR_RESPONSE_OFFSET    0x268
#define IOMMU_ICVEC_OFFSET          0x2f8
#define IOMMU_MSI_ADDR_0_OFFSET     0x300
#define IOMMU_MSI_DATA_0_OFFSET     0x308
#define IOMMU_MSI_VEC_CTL_0_OFFSET  0x30c
#define IOMMU_MSI_ADDR_1_OFFSET     0x310
#define IOMMU_MSI_DATA_1_OFFSET     0x318
#define IOMMU_MSI_VEC_CTL_1_OFFSET  0x31c
#define IOMMU_MSI_ADDR_2_OFFSET     0x320
#define IOMMU_MSI_DATA_2_OFFSET     0x328
#define IOMMU_MSI_VEC_CTL_2_OFFSET  0x32c
#define IOMMU_MSI_ADDR_3_OFFSET     0x330
#define IOMMU_MSI_DATA_3_OFFSET     0x338
#define IOMMU_MSI_VEC_CTL_3_OFFSET  0x33c
#define IOMMU_MSI_ADDR_4_OFFSET     0x340
#define IOMMU_MSI_DATA_4_OFFSET     0x348
#define IOMMU_MSI_VEC_CTL_4_OFFSET  0x34c
#define IOMMU_MSI_ADDR_5_OFFSET     0x350
#define IOMMU_MSI_DATA_5_OFFSET     0x358
#define IOMMU_MSI_VEC_CTL_5_OFFSET  0x35c
#define IOMMU_MSI_ADDR_6_OFFSET     0x360
#define IOMMU_MSI_DATA_6_OFFSET     0x368
#define IOMMU_MSI_VEC_CTL_6_OFFSET  0x36c
#define IOMMU_MSI_ADDR_7_OFFSET     0x370
#define IOMMU_MSI_DATA_7_OFFSET     0x378
#define IOMMU_MSI_VEC_CTL_7_OFFSET  0x37c
#define IOMMU_MSI_ADDR_8_OFFSET     0x380
#define IOMMU_MSI_DATA_8_OFFSET     0x388
#define IOMMU_MSI_VEC_CTL_8_OFFSET  0x38c
#define IOMMU_MSI_ADDR_9_OFFSET     0x390
#define IOMMU_MSI_DATA_9_OFFSET     0x398
#define IOMMU_MSI_VEC_CTL_9_OFFSET  0x39c
#define IOMMU_MSI_ADDR_10_OFFSET    0x3a0
#define IOMMU_MSI_DATA_10_OFFSET    0x3a8
#define IOMMU_MSI_VEC_CTL_10_OFFSET 0x3ac
#define IOMMU_MSI_ADDR_11_OFFSET    0x3b0
#define IOMMU_MSI_DATA_11_OFFSET    0x3b8
#define IOMMU_MSI_VEC_CTL_11_OFFSET 0x3bc
#define IOMMU_MSI_ADDR_12_OFFSET    0x3c0
#define IOMMU_MSI_DATA_12_OFFSET    0x3c8
#define IOMMU_MSI_VEC_CTL_12_OFFSET 0x3cc
#define IOMMU_MSI_ADDR_13_OFFSET    0x3d0
#define IOMMU_MSI_DATA_13_OFFSET    0x3d8
#define IOMMU_MSI_VEC_CTL_13_OFFSET 0x3dc
#define IOMMU_MSI_ADDR_14_OFFSET    0x3e0
#define IOMMU_MSI_DATA_14_OFFSET    0x3e8
#define IOMMU_MSI_VEC_CTL_14_OFFSET 0x3ec
#define IOMMU_MSI_ADDR_15_OFFSET    0x3f0
#define IOMMU_MSI_DATA_15_OFFSET    0x3f8
#define IOMMU_MSI_VEC_CTL_15_OFFSET 0x3fc

#define IOMMU_REG_ADDR(OFF)     (IOMMU_BASE_ADDR + OFF)

// cqcsr masks
#define CQCSR_CQEN          (1UL << 0)
#define CQCSR_CIE           (1UL << 1)
#define CQCSR_CQMF          (1UL << 8)
#define CQCSR_CMD_TO        (1UL << 9)
#define CQCSR_CMD_ILL       (1UL << 10)
#define CQCSR_FENCE_W_IP    (1UL << 11)
#define CQCSR_CQON          (1UL << 16)
#define CQCSR_BUSY          (1UL << 17)

#define IOFENCE_DATA    (0xABCDEFUL)

void init_iommu(void);

void set_iommu_off(void);
void set_iommu_bare(void);
void set_iommu_1lvl(void);
void set_ig_wsi();
void set_ig_msi();
uint32_t rv_iommu_get_ipsr();
void rv_iommu_clear_ipsr_fip();

/** HPM-related functions */
uint32_t rv_iommu_get_iocountovf();
uint32_t rv_iommu_get_iocountihn();
void rv_iommu_set_iocountihn(uint32_t iocountihn_new);
uint64_t rv_iommu_get_iohpmcycles();
void rv_iommu_set_iohpmcycles(uint64_t iohpmcycles_new);
uint64_t rv_iommu_get_iohpmctr (size_t counter_idx);
void rv_iommu_set_iohpmctr(uint64_t iohpmctr_new, size_t counter_idx);
uint64_t rv_iommu_get_iohpmevt (size_t counter_idx);
void rv_iommu_set_iohpmevt(uint64_t iohpmevt_new, size_t counter_idx);

/** Debug interface */
void rv_iommu_dbg_set_iova(uint64_t iova);
void rv_iommu_dbg_set_did(uint64_t device_id);
void rv_iommu_dbg_set_pv(bool pv);
void rv_iommu_dbg_set_priv(bool priv);
void rv_iommu_dbg_set_rw(bool rw);
void rv_iommu_dbg_set_exe(bool exe);
void rv_iommu_dbg_set_go(void);
bool rv_iommu_dbg_req_is_complete(void);
uint8_t rv_iommu_dbg_req_fault(void);
uint8_t rv_iommu_dbg_req_is_superpage(void);
uint64_t rv_iommu_dbg_translated_ppn(void);
uint8_t rv_iommu_dbg_ppn_encode_x(void);

/** Command-Queue-related functions */
void rv_iommu_cq_init(void);
uint32_t rv_iommu_get_cqh(void);
uint32_t rv_iommu_get_cqcsr(void);
void rv_iommu_set_cqcsr(uint32_t new_cqcsr);
void rv_iommu_induce_fault_cq(void);
void rv_iommu_ddt_inval(bool dv, uint64_t device_id);
void rv_iommu_iotinval_vma(bool av, bool gv, bool pscv, uint64_t addr, uint64_t gscid, uint64_t pscid);
void rv_iommu_iotinval_gvma(bool av, bool gv, uint64_t addr, uint64_t gscid);
void rv_iommu_iofence_c(bool wsi, bool av);
uint32_t rv_iommu_get_iofence(void);

#endif  /* _RV_IOMMU_H_ */