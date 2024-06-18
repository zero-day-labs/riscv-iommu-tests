#ifndef HPM_H
#define HPM_H

#include <rv_iommu_tests.h>

// Enabled counters
#define CNT_MASK    (0x0000003EUL)

// eventIDs
#define HPM_UT_REQ      (0x1ULL)
#define HPM_T_REQ       (0x2ULL)
#define HPM_ATS_REQ     (0x3ULL)
#define HPM_IOTLB_MISS  (0x4ULL)
#define HPM_DDTW        (0x5ULL)
#define HPM_PDTW        (0x6ULL)
#define HPM_S1_PTW      (0x7ULL)
#define HPM_S2_PTW      (0x8ULL)

// iohpmevt fields
#define IOHPMEVT_DMASK          (1ULL << 15)
#define IOHPMEVT_PID_PSCID_OFF  (16)
#define IOHPMEVT_PID_PSCID_MASK (0xFFFFF0000ULL)
#define IOHPMEVT_DID_GSCID_OFF  (36)
#define IOHPMEVT_DID_GSCID_MASK (0xFFFFFF000000000ULL)
#define IOHPMEVT_PV_PSCV        (1ULL << 60)
#define IOHPMEVT_DV_GSCV        (1ULL << 61)
#define IOHPMEVT_IDT            (1ULL << 62)
#define IOHPMEVT_OF             (1ULL << 63)

#endif  /* HPM_H*/