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

#define PAGE_SIZE           0x1000ULL     // 4kiB

// iDMA modules generate an interrupt after completing a transfer
#define IDMA_IRQ_EN     (0)

// The IOMMU IP supports MSI translation
// If set, DCs are configured in extended format
#define MSI_TRANSLATION (1)

// Min and max device IDs of DMA devices in the platform.
// DDT entries are created for device IDs in [DID_MIN, DID_MAX]
#define DID_MIN             (1)
#define DID_MAX             (15)

// Interrupt vectors
#define CQ_INT_VECTOR       (0x03ULL)
#define FQ_INT_VECTOR       (0x02ULL)
#define HPM_INT_VECTOR      (0x01ULL)

// Interrupt pending bits
#define CIP_MASK            (1UL << 0)
#define FIP_MASK            (1UL << 1)
#define PMIP_MASK           (1UL << 2)

// Number of transfers for latency test
#define N_TRANSFERS         (200)

// Number of mappings (PTEs) used for the latency test
#define N_MAPPINGS          (32)

typedef uint64_t pte_t;

#endif /* IOMMU_TESTS_H */