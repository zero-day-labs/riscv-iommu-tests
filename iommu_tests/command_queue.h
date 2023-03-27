#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <iommu_tests/iommu_tests.h>

// Number of entries in the CQ. Must be POT
#define CQ_N_ENTRIES    (64 )
// Size of the queue represented as Log2(64)-1 = 5
#define CQ_LOG2SZ_1     (5  )

// Mask for cqb.PPN (cqb[53:10])
#define CQB_PPN_MASK    (0x3FFFFFFFFFFC00ULL)
// Offset for cqb.PPN
#define CQB_PPN_OFF     (10)

// cqcsr masks
#define CQCSR_CQEN          (1ULL << 0)
#define CQCSR_CIE           (1ULL << 1)
#define CQCSR_CQMF          (1ULL << 8)
#define CQCSR_CMD_TO        (1ULL << 9)
#define CQCSR_CMD_ILL       (1ULL << 10)
#define CQCSR_FENCE_W_IP    (1ULL << 11)
#define CQCSR_CQON          (1ULL << 16)
#define CQCSR_BUSY          (1ULL << 17)

// TODO: If using this struct, update fields to the current spec
typedef union {
    struct {
        uint64_t opcode:7;
        uint64_t func3:3;
        uint64_t av:1;
        uint64_t rsvd1:1;
        uint64_t pscid:20;
        uint64_t pscv:1;
        uint64_t gv:1;
        uint64_t rsvd2:10;
        uint64_t gscid:16;
        uint64_t rsvd3:4;
        uint64_t rsvd4:10;
        uint64_t addr_63_12:52;
        uint64_t rsvd5:2;
    } iotinval;
    struct {
        uint64_t opcode:7;
        uint64_t func3:3;
        uint64_t dv:1;
        uint64_t rsvd:5;
        uint64_t pid:20;
        uint64_t rsvd1:4;
        uint64_t did:24;
        uint64_t rsvd2;
    } iodir;
    struct {
        uint64_t opcode:7;
        uint64_t func3:3;
        uint64_t pr:1;
        uint64_t pw:1;
        uint64_t av:1;
        uint64_t wis:1;
        uint64_t reserved:18;
        uint64_t data:32;
        uint64_t reserved1:2;
        uint64_t addr_63_2:62;
    } iofence;
    struct {
        uint64_t opcode:7;
        uint64_t func3:3;
        uint64_t dsv:1;
        uint64_t pv:1;
        uint64_t reserved:4;
        uint64_t pid:20;
        uint64_t rsvd1:4;
        uint64_t dseg:8;
        uint64_t rid:16;
        uint64_t payload;
    } ats;
    struct {
        uint64_t low;
        uint64_t high;
    };
} command_t;

extern uint64_t command_queue[];

void cq_init(void);

#endif  /* COMMAND_QUEUE_H*/