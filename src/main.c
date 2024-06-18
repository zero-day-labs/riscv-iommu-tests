#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <rvh_test.h>
#include <iommu_tests.h>
#include <rv_iommu.h>

void main(){

    INFO("RISC-V Input/Output Memory Management Unit Tests");

    // Reset CPU
    reset_state();
    init_iommu();

    // Test functions are manually assigned to the .test_table 
    // section in the test_register.c file using the TEST_REGISTER macro
    for(int i = 0; i < test_table_size; i++)
        test_table[i]();

    END();
}
