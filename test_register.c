#include <rvh_test.h>

/**
 * # Sequence:
 * 
 * Configure CQ, FQ, S1 and S2 page tables, MSI page tables, ddtp (IOMMU off and translation in bare mode)
 * Perform transfer and check fault
 * Test IOMMU in Bare mode using physical addresses to perform memory transfers.
 * Program the ddtp to enable the IOMMU in 1LVL mode.
 * Perform a transfer with both translation stages in Bare mode using an SPA
 * Change the DC to enable second-stage translation and MSI translation.
 * Perform a memory transfer using normal GPA and an MSI GPA.
 * Change the DC to enable first-stage translation.
 * Perform a memory transfer using normal GVA and an MSI GVA.
 * Test fault reporting performing a memory transfer with wrong permissions
 * Test IOTLB invalidation. Perform two transfers. Swap one stage PTs. Check if values Swapped.
 * Change the other stage PTs. Check whether the values swapped again.
 * 
 * Test WSI generation by inducing FQ records
 * Test IOFENCE command with WSI generation and MSG writing
 * Program the IOMMU to generate MSIs
 * Test MSI generation issuing misconfigured commands to the CQ and generating FQ records.
 * 
 */

TEST_REGISTER(stress_latency);
// TEST_REGISTER(hpm);
// TEST_REGISTER(msi_generation);
// TEST_REGISTER(iofence);
// TEST_REGISTER(wsi_generation);
// TEST_REGISTER(iotinval);
// TEST_REGISTER(two_stage_translation);
// TEST_REGISTER(second_stage_only);
// TEST_REGISTER(both_stages_bare);
// TEST_REGISTER(iommu_bare);
// TEST_REGISTER(iommu_off);


// TEST_REGISTER(idma_only_multiple_beats);
// TEST_REGISTER(idma_only);
