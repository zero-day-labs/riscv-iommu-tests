#ifndef IOMMU_TESTS_H
#define IOMMU_TESTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <csrs.h>
#include <instructions.h>
#include <platform.h>
#include <rvh_test.h>

#define CQ_INT_VECTOR       (0x03ULL)
#define FQ_INT_VECTOR       (0x02ULL)
#define HPM_INT_VECTOR      (0x01ULL)

#define CIP_MASK            (1UL << 0)
#define FIP_MASK            (1UL << 1)
#define PMIP_MASK           (1UL << 2)

#define PAGE_SIZE           0x1000ULL     // 4kiB

// Number of transfers for stress latency test
#define N_TRANSFERS         (100)

typedef uint64_t pte_t;

#endif /* IOMMU_TESTS_H */