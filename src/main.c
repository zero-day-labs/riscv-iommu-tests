#include <rvh_test.h>
#include <rv_iommu.h>

void main(){

    INFO("RISC-V Input/Output Memory Management Unit Tests");

    // Reset CPU
    reset_state();
    // Init IOMMU with basic configuration
    init_iommu();

    // Test functions are manually assigned to the .test_table 
    // section in the test_register.c file using the TEST_REGISTER macro
    for(int i = 0; i < test_table_size; i++)
        test_table[i]();

    END();
}
