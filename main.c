#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <rvh_test.h>

bool check_csr_field_spec(){

    TEST_START();

    /* this assumes machine mode */
    //check_csr_wrrd("mstatus", mstatus, (uint64_t) -1, 0x800000ca007e79aaULL);
    //check_csr_wrrd("mideleg", mideleg, (uint64_t) -1, 0x1666);
    //check_csr_wrrd("medeleg", medeleg, (uint64_t) -1, 0xb15d);
    check_csr_wrrd("mideleg", mideleg, (uint64_t)0, 0x444);
    //check_csr_wrrd("mip", mip, (uint64_t) -1, 0x6e6);
    //check_csr_wrrd("mie", mie, (uint64_t) -1, 0x1eee);
    check_csr_wrrd("mtinst", CSR_MTINST, (uint64_t) -1, (uint64_t) -1);
    check_csr_wrrd("mtval2", CSR_MTVAL2, (uint64_t) -1, (uint64_t) -1);
    //check_csr_wrrd("hstatus", CSR_HSTATUS, (uint64_t) -1, 0x30053f3e0);
    //check_csr_wrrd("hideleg", CSR_HIDELEG, (uint64_t) -1, 0x444);
    //check_csr_wrrd("hedeleg", CSR_HEDELEG, (uint64_t) -1, 0xb1ff);
    //check_csr_wrrd("hvip", CSR_HVIP, (uint64_t) -1, 0x444);
    //check_csr_wrrd("hip", CSR_HIP, (uint64_t) -1, 0x4);
    check_csr_wrrd("hie", CSR_HIE, (uint64_t) -1, 0x444);
    check_csr_wrrd("htval", CSR_HTVAL, (uint64_t) -1, (uint64_t) -1);
    check_csr_wrrd("htinst", CSR_HTINST, (uint64_t) -1, (uint64_t) -1);
    //check_csr_wrrd("hgatp", CSR_HGATP, (8ULL << 60) | (1ULL << 60)-1, 0x80000000000fffffULL);
    //check_csr_wrrd("vsstatus", CSR_VSSTATUS, (uint64_t) -1, 0x80000000000c6122ULL);
    //check_csr_wrrd("vsip", CSR_VSIP, (uint64_t) -1, 0x0);
    //check_csr_wrrd("vsie", CSR_VSIE, (uint64_t) -1, 0x0);
    //check_csr_wrrd("vstvec", CSR_VSTVEC, (uint64_t) -1, 0xffffffffffffff01ULL);
    check_csr_wrrd("vsscratch", CSR_VSSCRATCH, (uint64_t) -1, (uint64_t) -1);
    //check_csr_wrrd("vsepc", CSR_VSEPC, (uint64_t) -1, 0xfffffffffffffffeULL);
    //check_csr_wrrd("vscause", CSR_VSCAUSE, (uint64_t) -1, 0x800000000000001fULL);
    check_csr_wrrd("vstval", CSR_VSTVAL, (uint64_t) -1, 0xffffffffffffffffULL);
    //check_csr_wrrd("vsatp", CSR_VSATP, (uint64_t) -1, 0x0);
    //check_csr_wrrd("vsatp", CSR_VSATP, (8ULL << 60) | (1ULL << 60)-1, 0x80000000000fffffULL);

    TEST_END();
}

/**
 *   Check bit 7 of misa CSR
 */
bool check_misa_h(){

    // Print "check_misa_h"
    TEST_START();

    // Read value from misa to restore later
    uint64_t misa = CSRR(misa);
    // Set bit 7 of misa (Hypervisor Extension)
    CSRS(misa, (1ULL << 7));

    // Read misa and test bit 7
    bool hyp_ext_present = CSRR(misa) & (1ULL << 7);

    // If bit 7 was not set, hypervisor extension is not present
    TEST_ASSERT("check h bit after setting it",  hyp_ext_present, "hypervisor extensions not present");

    if(!hyp_ext_present)
        return false;

    // Bit was set. Try to clear it to check whether it is hardwired to one
    CSRC(misa, (1ULL << 7));
    if(((CSRR(misa) & (1ULL << 7)))){
        VERBOSE("misa h bit is hardwired");
    }

    // Restore original value of misa
    CSRW(misa, misa);

    TEST_END();
}

void main(){

    INFO("RISC-V Hypervisor Extensions tests");

    // Check for bit 7 of misa CSR
    if(check_misa_h()){

        // Reset CPU
        reset_state();

        // Test functions are manually assigned to the .test_table 
        // section in the test_register.c file using the TEST_REGISTER macro
        for(int i = 0; i < test_table_size; i++)
            test_table[i]();
    }

    INFO("end");
    exit(0);
}
