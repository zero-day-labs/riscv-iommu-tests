#include <idma.h>
#include <rvh_test.h>

/**
 *  iDMA device IDs 
 */
uint64_t idma_ids[N_DMA] = {
    1ULL,
    2ULL,
    3ULL,
    4ULL
};

/**
 *  iDMA Configuration Registers
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

/**
 *  Setup iDMA for transfer
 */
void idma_setup(int idx, uint64_t src, uint64_t dst, uint64_t n_bytes)
{
    write64(idma_src[idx], src);          // Source address
    write64(idma_dest[idx], dst);         // Destination address
    write64(idma_nbytes[idx], n_bytes);   // N of bytes to be transferred
    write64(idma_config[idx], 0);         // iDMA config: Disable decouple, deburst and serialize
}

/**
 *  Setup iDMA src and dst addresses only
 */
void idma_setup_addr(int idx, uint64_t src, uint64_t dst)
{
    write64(idma_src[idx], src);          // Source address
    write64(idma_dest[idx], dst);         // Destination address
}

/**
 *  Init iDMA transfer and wait for completion 
 */
int idma_exec_transfer(int idx)
{
    uint64_t trans_id = read64(idma_nextid[idx]);

    if (!trans_id)
        return -1;

    // Poll transfer status
    while (read64(idma_done[idx]) != trans_id)
        ;

    return 0;
}