#ifndef IOMMU_TESTS_H
#define IOMMU_TESTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <csrs.h>
#include <instructions.h>
#include <platform.h>
#include <rvh_test.h>

// IOMMU Mode
#define IOMMU_OFF           (0)   // Turn off IOMMU
#define IOMMU_BARE          (0)   // Set IOMMU to bare (bypass transactions)

// device_id width assumed to be AXI ID width
// 6, 15, 24
#define DEVICE_ID_WIDTH     (6)

// Device Context Format (1 for extended format, 0 for base format)
#define DC_EXT_FORMAT       (1)

#define CQ_INT_VECTOR       (0x03ULL)
#define FQ_INT_VECTOR       (0x0AULL)

#define CIP_MASK            (1UL << 0)
#define FIP_MASK            (1UL << 1)

#define PAGE_SIZE           0x1000ULL     // 4kiB

// Base address of the IOMMU Programming Interface
#define IOMMU_BASE_ADDR             0x50010000ULL

// Register Offsets
#define IOMMU_CAPABILITIES_OFFSET   0x0
#define IOMMU_FCTL_OFFSET           0x8
#define IOMMU_DDTP_OFFSET           0x10
#define IOMMU_CQB_OFFSET            0x18
#define IOMMU_CQH_OFFSET            0x20
#define IOMMU_CQT_OFFSET            0x24
#define IOMMU_FQB_OFFSET            0x28
#define IOMMU_FQH_OFFSET            0x30
#define IOMMU_FQT_OFFSET            0x34
#define IOMMU_CQCSR_OFFSET          0x48
#define IOMMU_FQCSR_OFFSET          0x4c
#define IOMMU_IPSR_OFFSET           0x54
#define IOMMU_ICVEC_OFFSET          0x2f8
#define IOMMU_MSI_ADDR_0_OFFSET     0x300
#define IOMMU_MSI_DATA_0_OFFSET     0x308
#define IOMMU_MSI_VEC_CTL_0_OFFSET  0x30c
#define IOMMU_MSI_ADDR_1_OFFSET     0x310
#define IOMMU_MSI_DATA_1_OFFSET     0x318
#define IOMMU_MSI_VEC_CTL_1_OFFSET  0x31c
#define IOMMU_MSI_ADDR_2_OFFSET     0x320
#define IOMMU_MSI_DATA_2_OFFSET     0x328
#define IOMMU_MSI_VEC_CTL_2_OFFSET  0x32c
#define IOMMU_MSI_ADDR_3_OFFSET     0x330
#define IOMMU_MSI_DATA_3_OFFSET     0x338
#define IOMMU_MSI_VEC_CTL_3_OFFSET  0x33c
#define IOMMU_MSI_ADDR_4_OFFSET     0x340
#define IOMMU_MSI_DATA_4_OFFSET     0x348
#define IOMMU_MSI_VEC_CTL_4_OFFSET  0x34c
#define IOMMU_MSI_ADDR_5_OFFSET     0x350
#define IOMMU_MSI_DATA_5_OFFSET     0x358
#define IOMMU_MSI_VEC_CTL_5_OFFSET  0x35c
#define IOMMU_MSI_ADDR_6_OFFSET     0x360
#define IOMMU_MSI_DATA_6_OFFSET     0x368
#define IOMMU_MSI_VEC_CTL_6_OFFSET  0x36c
#define IOMMU_MSI_ADDR_7_OFFSET     0x370
#define IOMMU_MSI_DATA_7_OFFSET     0x378
#define IOMMU_MSI_VEC_CTL_7_OFFSET  0x37c
#define IOMMU_MSI_ADDR_8_OFFSET     0x380
#define IOMMU_MSI_DATA_8_OFFSET     0x388
#define IOMMU_MSI_VEC_CTL_8_OFFSET  0x38c
#define IOMMU_MSI_ADDR_9_OFFSET     0x390
#define IOMMU_MSI_DATA_9_OFFSET     0x398
#define IOMMU_MSI_VEC_CTL_9_OFFSET  0x39c
#define IOMMU_MSI_ADDR_10_OFFSET    0x3a0
#define IOMMU_MSI_DATA_10_OFFSET    0x3a8
#define IOMMU_MSI_VEC_CTL_10_OFFSET 0x3ac
#define IOMMU_MSI_ADDR_11_OFFSET    0x3b0
#define IOMMU_MSI_DATA_11_OFFSET    0x3b8
#define IOMMU_MSI_VEC_CTL_11_OFFSET 0x3bc
#define IOMMU_MSI_ADDR_12_OFFSET    0x3c0
#define IOMMU_MSI_DATA_12_OFFSET    0x3c8
#define IOMMU_MSI_VEC_CTL_12_OFFSET 0x3cc
#define IOMMU_MSI_ADDR_13_OFFSET    0x3d0
#define IOMMU_MSI_DATA_13_OFFSET    0x3d8
#define IOMMU_MSI_VEC_CTL_13_OFFSET 0x3dc
#define IOMMU_MSI_ADDR_14_OFFSET    0x3e0
#define IOMMU_MSI_DATA_14_OFFSET    0x3e8
#define IOMMU_MSI_VEC_CTL_14_OFFSET 0x3ec
#define IOMMU_MSI_ADDR_15_OFFSET    0x3f0
#define IOMMU_MSI_DATA_15_OFFSET    0x3f8
#define IOMMU_MSI_VEC_CTL_15_OFFSET 0x3fc

#define IOMMU_REG_ADDR(OFF)     (IOMMU_BASE_ADDR + OFF)

//# iDMA

#define IDMA_DECOUPLE   (1ULL << 0)
#define IDMA_DEBURST    (1ULL << 1)
#define IDMA_SERIALIZE  (1ULL << 2)

// Base address of the iDMA programming interface
#define IDMA_BASE_ADDR             0x50000000ULL

// Register offsets
#define IDMA_SRC_ADDR   0x0
#define IDMA_DEST_ADDR  0x8
#define IDMA_N_BYTES    0x10
#define IDMA_CONFIG     0x18
#define IDMA_STATUS     0x20
#define IDMA_NEXT_ID    0x28
#define IDMA_DONE       0x30

#define IDMA_REG_ADDR(OFF)      (IDMA_BASE_ADDR + OFF)

typedef uint64_t pte_t;

void init_iommu(void);


#endif /* IOMMU_TESTS_H */