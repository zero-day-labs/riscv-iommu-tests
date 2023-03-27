#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <iommu_tests/iommu_tests.h>

// Number of entries in the FQ. Must be POT
#define FQ_N_ENTRIES    (64 )
// Size of the queue represented as Log2(64)-1 = 5
#define FQ_LOG2SZ_1     (5  )

// Mask for cqb.PPN (fqb[53:10])
#define FQB_PPN_MASK    (0x3FFFFFFFFFFC00ULL)
// Offset for fqb.PPN
#define FQB_PPN_OFF     (10)

// fqcsr masks
#define FQCSR_FQEN      (1ULL << 0)
#define FQCSR_FIE       (1ULL << 1)
#define FQCSR_FQMF      (1ULL << 8)
#define FQCSR_FQOF      (1ULL << 9)
#define FQCSR_FQON      (1ULL << 16)
#define FQCSR_BUSY      (1ULL << 17)

void fq_init(void);

#endif  /* COMMAND_QUEUE_H*/