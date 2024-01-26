#ifndef DBG_IH_H
#define DBG_IH_H

#include <iommu_tests.h>
#include <rv_iommu.h>

#define TR_REQ_CTL_DID_OFFSET   40
#define TR_REQ_CTL_DID_MASK     0xFFFFFF0000000000ULL
#define TR_REQ_CTL_PID_OFFSET   12
#define TR_REQ_CTL_PID_MASK     0xFFFFF000ULL
#define TR_REQ_CTL_PV_BIT       (1ULL << 32)
#define TR_REQ_CTL_NW_BIT       (1ULL << 3)
#define TR_REQ_CTL_EXE_BIT      (1ULL << 2)
#define TR_REQ_CTL_PRIV_BIT     (1ULL << 1)
#define TR_REQ_CTL_GO_BIT       (1ULL << 0)

#define TR_RESPONSE_FAULT_BIT   (1ULL << 0)
#define TR_RESPONSE_SP_BIT      (1ULL << 9)
#define TR_RESPONSE_PPN_OFFSET  (10)
#define TR_RESPONSE_PPN_MASK    (0x3FFFFFFFFFFC00ULL)


void dbg_set_iova(uint64_t iova);
void dbg_set_did(uint64_t device_id);
void dbg_set_pv(bool pv);
void dbg_set_priv(bool is_priv);
void dbg_set_rw(bool is_rw);
void dbg_set_exe(bool is_exe);
void dbg_trigger_translation(void);
bool dbg_is_complete(void);
uint8_t dbg_is_fault(void);
uint8_t dbg_is_superpage(void);
uint64_t dbg_translated_ppn(void);
uint8_t dbg_ppn_encode_x(void);

#endif  /* DBG_IH_H */