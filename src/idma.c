#include <idma.h>
#include <rvh_test.h>

/**
 *  Setup iDMA for transfer
 */
void idma_setup(struct idma *dma_ut, uint64_t src, uint64_t dst, uint64_t n_bytes)
{
    write64((uintptr_t)&dma_ut->src_addr, src);          // Source address
    write64((uintptr_t)&dma_ut->dest_addr, dst);         // Destination address
    write64((uintptr_t)&dma_ut->num_bytes, n_bytes);   // N of bytes to be transferred
    write64((uintptr_t)&dma_ut->config, ~(IDMA_SERIALIZE|IDMA_DEBURST|IDMA_DECOUPLE));         // iDMA config: Disable decouple, deburst and serialize
}

/**
 *  Setup iDMA src and dst addresses only
 */
void idma_setup_addr(struct idma *dma_ut, uint64_t src, uint64_t dst)
{
    write64((uintptr_t)&dma_ut->src_addr, src);          // Source address
    write64((uintptr_t)&dma_ut->dest_addr, dst);         // Destination address
}

/**
 *  Init iDMA transfer and wait for completion 
 */
int idma_exec_transfer(struct idma *dma_ut)
{
    uint64_t trans_id = read64((uintptr_t)&dma_ut->next_transfer_id);

    if (!trans_id)
        return -1;

    // Poll transfer status
    while (read64((uintptr_t)&dma_ut->last_transfer_id_complete) != trans_id)
        ;

    return 0;
}