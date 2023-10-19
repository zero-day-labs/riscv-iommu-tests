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
#include <iommu.h>
#include <rvh_test.h>

// iDMA modules generate an interrupt after completing a transfer
#define IDMA_IRQ_EN     (1)

// MSI translation support
#define MSI_TRANSLATION (1)

// Min and max device ID of DMA devices in the platform. Used to fill DDT
#define DID_MIN             (1)
#define DID_MAX             (4)

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