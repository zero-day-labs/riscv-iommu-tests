#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <iommu_tests.h>

// Number of entries in the CQ. Must be POT
#define CQ_N_ENTRIES    (64 )
// Size of the queue represented as Log2(64)-1 = 5
#define CQ_LOG2SZ_1     (5  )

// Mask for cqb.PPN (cqb[53:10])
#define CQB_PPN_MASK    (0x3FFFFFFFFFFC00ULL)
// Mask for ADDR[63:12]
#define ADDR_63_12_MASK (0xFFFFFFFFFFFFF000ULL)
// Offset for cqb.PPN
#define CQB_PPN_OFF     (10)

// opcodes
#define IOTINVAL    (0x1ULL << 0)
#define IOFENCE     (0x2ULL << 0)
#define IODIR       (0x3ULL << 0)

// func3
#define VMA         (0ULL << 7)
#define GVMA        (1ULL << 7)
#define FUNC3_C   (0ULL << 7)
#define INVAL_DDT   (0ULL << 7)
#define INVAL_PDT   (1ULL << 7)

// misc fields
#define IOTINVAL_AV     (1ULL << 10)
#define IOTINVAL_GV     (1ULL << 33)
#define IOTINVAL_PSCV   (1ULL << 32)

#define IOFENCE_AV      (1ULL << 10)
#define IOFENCE_WSI     (1ULL << 11)
#define IOFENCE_PR      (1ULL << 12)
#define IOFENCE_PW      (1ULL << 13)
#define IOFENCE_DATA    (0xABCDEFUL)
#define IOFENCE_ADDR    (0x82000000ULL)

#define IODIR_DV        (1ULL << 33)

// offsets
#define IOTINVAL_PSCID_OFF  (12)
#define IOTINVAL_GSCID_OFF  (44)
#define IOTINVAL_IOVA_OFF   (10)

#define IODIR_DID_OFF       (40)

// cqcsr masks
#define CQCSR_CQEN          (1UL << 0)
#define CQCSR_CIE           (1UL << 1)
#define CQCSR_CQMF          (1UL << 8)
#define CQCSR_CMD_TO        (1UL << 9)
#define CQCSR_CMD_ILL       (1UL << 10)
#define CQCSR_FENCE_W_IP    (1UL << 11)
#define CQCSR_CQON          (1UL << 16)
#define CQCSR_BUSY          (1UL << 17)

extern uint64_t command_queue[];

void cq_init(void);

#endif  /* COMMAND_QUEUE_H*/