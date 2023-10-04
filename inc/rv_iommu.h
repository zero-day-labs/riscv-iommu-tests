#ifndef _RV_IOMMU_H_
#define _RV_IOMMU_H_

#include <iommu_tests.h>

// Mask for ddtp.PPN (ddtp[53:10])
#define DDTP_PPN_MASK    (0x3FFFFFFFFFFC00ULL)

// Define DDT (IOMMU) mode
#define DDTP_MODE_OFF   (0ULL)
#define DDTP_MODE_BARE  (1ULL)
#define DDTP_MODE_1LVL  (2ULL)
#define DDTP_MODE_2LVL  (3ULL)
#define DDTP_MODE_3LVL  (4ULL)

// MSI translation support
#define MSI_TRANSLATION (1)

// Base address of the IOMMU Programming Interface
#define IOMMU_BASE_ADDR             0x50010000ULL

// IOMMU Register Offsets
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
#define IOMMU_IOCOUNTOVF_OFFSET     0x58
#define IOMMU_IOCOUNTINH_OFFSET     0x5c
#define IOMMU_IOHPMCYCLES_OFFSET    0x60
#define IOMMU_IOHPMCTR_OFFSET       0x68
#define IOMMU_IOHPMEVT_OFFSET       0x160
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

void init_iommu(void);

void set_iommu_off(void);
void set_iommu_bare(void);
void set_iommu_1lvl(void);
void set_ig_wsi();
void set_ig_msi();

#endif  /* _RV_IOMMU_H_ */