#include <dbg_if.h>

uintptr_t tr_req_iova_addr = IOMMU_REG_ADDR(IOMMU_TR_REQ_IOVA_OFFSET);
uintptr_t tr_req_ctl_addr = IOMMU_REG_ADDR(IOMMU_TR_REQ_CTL_OFFSET);
uintptr_t tr_response_addr = IOMMU_REG_ADDR(IOMMU_TR_RESPONSE_OFFSET);

void dbg_set_iova(uint64_t iova)
{
    uint64_t tr_req_iova = (iova & ~0xFFFULL);
    write64(tr_req_iova_addr, tr_req_iova);
}

void dbg_set_did(uint64_t device_id)
{
    uint64_t tr_req_ctl = read64(tr_req_ctl_addr);
    tr_req_ctl |= ((device_id << TR_REQ_CTL_DID_OFFSET) & TR_REQ_CTL_DID_MASK);

    write64(tr_req_ctl_addr, tr_req_ctl);
}

void dbg_set_pv(bool pv)
{
    uint64_t tr_req_ctl = read64(tr_req_ctl_addr);
    if (pv)
        tr_req_ctl |= TR_REQ_CTL_PV_BIT;
    else
        tr_req_ctl &= (~TR_REQ_CTL_PV_BIT);

    write64(tr_req_ctl_addr, tr_req_ctl);
}

void dbg_set_priv(bool is_priv)
{
    uint64_t tr_req_ctl = read64(tr_req_ctl_addr);
    if (is_priv)
        tr_req_ctl |= TR_REQ_CTL_PRIV_BIT;
    else
        tr_req_ctl &= (~TR_REQ_CTL_PRIV_BIT);

    write64(tr_req_ctl_addr, tr_req_ctl);
}

void dbg_set_rw(bool is_rw)
{
    uint64_t tr_req_ctl = read64(tr_req_ctl_addr);
    if (is_rw)
        tr_req_ctl &= (~TR_REQ_CTL_EXE_BIT);
    else
        tr_req_ctl |= TR_REQ_CTL_EXE_BIT;

    write64(tr_req_ctl_addr, tr_req_ctl);
}

void dbg_set_exe(bool is_exe)
{
    uint64_t tr_req_ctl = read64(tr_req_ctl_addr);
    if (is_exe)
        tr_req_ctl |= TR_REQ_CTL_EXE_BIT;
    else
        tr_req_ctl &= (~TR_REQ_CTL_EXE_BIT);

    write64(tr_req_ctl_addr, tr_req_ctl);
}

void dbg_trigger_translation(void)
{
    uint64_t tr_req_ctl = read64(tr_req_ctl_addr);
    tr_req_ctl |= TR_REQ_CTL_GO_BIT;

    write64(tr_req_ctl_addr, tr_req_ctl);
}

bool dbg_is_complete()
{
    uint64_t tr_req_ctl = read64(tr_req_ctl_addr);
    
    return ((tr_req_ctl & TR_REQ_CTL_GO_BIT != 0) ? (false) : (true));
}

bool dbg_is_fault()
{
    uint64_t tr_response = read64(tr_response_addr);

    return ((tr_response & TR_RESPONSE_FAULT_BIT != 0) ? (true) : (false));
}

bool dbg_is_superpage()
{
    uint64_t tr_response = read64(tr_response_addr);

    return ((tr_response & TR_RESPONSE_SP_BIT != 0) ? (true) : (false));
}

uint64_t dbg_translated_ppn()
{
    uint64_t tr_response = read64(tr_response_addr);

    return ((tr_response & TR_RESPONSE_PPN_MASK) >> TR_RESPONSE_PPN_OFFSET);
}

uint8_t dbg_ppn_encode_x()
{
    uint64_t tr_response = read64(tr_response_addr);
    uint8_t bit = 0, x_idx = 0;

    uint64_t ppn = (tr_response & TR_RESPONSE_PPN_MASK) >> TR_RESPONSE_PPN_OFFSET;
    while ((bit = (uint8_t)(ppn & 0x01ULL)) != 0)
    {
        ppn = (ppn >> 1);
        x_idx++;
    }
    
    return x_idx;
}
