#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <rvh_test.h>
#include <iommu_tests.h>
#include <rv_iommu.h>

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
    TEST_ASSERT("Checking misa.H after setting it",  hyp_ext_present, "Hypervisor extensions not present");

    if(!hyp_ext_present)
        return false;

    // Bit was set. Try to clear it to check whether it is hardwired to one
    CSRC(misa, (1ULL << 7));
    if(((CSRR(misa) & (1ULL << 7)))){
        VERBOSE("misa.H bit is hardwired to 1");
    }

    // Restore original value of misa
    CSRW(misa, misa);

    TEST_END();
}

void main(){

    INFO("RISC-V Input/Output Memory Management Unit Tests");

    // Reset CPU
    reset_state();
    init_iommu();

    // Test functions are manually assigned to the .test_table 
    // section in the test_register.c file using the TEST_REGISTER macro
    for(int i = 0; i < test_table_size; i++)
        test_table[i]();

    INFO("end");
    exit(0);
}
