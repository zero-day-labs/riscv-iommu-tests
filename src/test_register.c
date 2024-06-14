#include <rvh_test.h> // necessary to use TEST_REGISTER macro

/**
 *  List of tests.
 *  To disable a test from the application, comment the corresponding line
 */

// IOMMU latency test
// TEST_REGISTER(latency_test);

// IOMMU Arch tests
// TEST_REGISTER(dbg_interface);
// TEST_REGISTER(mrif_support);
// TEST_REGISTER(hpm);
// TEST_REGISTER(msi_generation);
// TEST_REGISTER(iofence);
// TEST_REGISTER(wsi_generation);
// TEST_REGISTER(iotinval);
// TEST_REGISTER(two_stage_translation);
// TEST_REGISTER(second_stage_only);
// TEST_REGISTER(both_stages_bare);
TEST_REGISTER(iommu_bare);
// TEST_REGISTER(iommu_off);

// iDMA-only tests
// TEST_REGISTER(idma_only_multiple_beats);
// TEST_REGISTER(idma_only);
