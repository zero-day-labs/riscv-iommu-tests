# RISCV IOMMU Architectural Tests

## License

This work is licensed under the Apache-2.0 License. See the [LICENSE](./LICENSE) file for details.

## Table of Contents

- [About this Project](#about-this-project)
- [Configuring the Tests](#configuring-the-tests)
- [Building the application](#building-the-application)

***

## About this Project 
This repository hosts a framework of architectural tests for the RISC-V IOMMU. It was developed as an adaptation of the [RISC-V Hypervisor extension tests](https://github.com/ninolomata/riscv-hyp-tests) framework to test a [RISC-V IOMMU IP](https://github.com/zero-day-labs/riscv-iommu) within a [CVA6-based SoC](https://github.com/zero-day-labs/cva6/tree/feat/iommu). To perform DMA transfers, we use the [PULP iDMA](https://github.com/pulp-platform/iDMA) module.

The Table below lists the included tests. Each one addresses one or more architectural features conforming to the [RISC-V IOMMU Specification](https://github.com/riscv-non-isa/riscv-iommu). All tests can be individually enabled or disabled before compilation.

| Test | Description |
|-|-|
|||
|**IOMMU Tests**|
| **iommu_off** | Test the IOMMU in *OFF* mode, using a DMA transfer and checking the fault record written into the FQ (should be *All inbound transactions disallowed*).|
| **iommu_bare** | Test the IOMMU in *Bare* mode (bypass), using a DMA transfer with supervisor physical addresses. |
| **both_stages_bare** |  Test address translation with first and second-stage in *Bare* mode, using a DMA transfer with supervisor physical addresses. |
| **second_stage_only** | Test *Sv39x4* second-stage translation with two DMA transfers. The second DMA transfer triggers MSI translation, if supported.|
| **two_stage_translation** | Test *Sv39/Sv39x4* two-stage translation using four DMA transfers: (i) 4-kiB pages (normal); (ii) 2-MiB superpages; (iii) 1-GiB superpages; and (iv) MSI translation.|
| **iotinval** | Test *IOTINVAL.VMA* and *IOTINVAL.GVMA* invalidation commands.|
| **wsi_generation** | Test fault recording and WSI generation using a misconfigured transfer.|
| **iofence** | Issue an *IOFENCE.C* command, with WSI and AV set to 1. Check MSI transfer and fence_w_ip bit.|
| **msi_generation** | Test MSI generation using a misconfigured transfer and an illegal command.|
| **hpm** | Test counter overflow and interrupt generation (WSI/MSI) in the free clock cycle counter and event counters.|
|||
|**iDMA Tests**||
| **idma_only**| Test SoC with iDMA module directly connected to the XBAR, i.e., without IOMMU.|
| **idma_only_multiple_beats**|Test multi-beat transfers in the SoC with iDMA module directly connected to the XBAR.|

## Configuring the Tests
Before building the application, you must configure some parameters according to the properties of your platform.

- In the [iommu_tests.h](./inc/iommu_tests.h) file, you can configure some IOMMU-related information. For example, **DID_MIN** and **DID_MAX** define the range of DDT entries that will be created to execute the tests. You must configure these values according to the device IDs of the DMAs present in your platform.

- You can disable/enable individual tests by commenting/uncommenting the corresponding line in the [test_register.c](./test_register.c) file.

- The base address of the programming interfaces of the IOMMU IP and the iDMA devices must be specified in **platform/`${PLAT}`/inc/iommu.h**

## Building the application
:information_source: The **riscv64-unknown-elf-** toolchain must be in your `${PATH}` to build the application.

### Target platform

The target platform on which the test will run must be specified by setting the `${PLAT}` environment variable. Currently, we only support the [CVA6-based platform](https://github.com/zero-day-labs/cva6/tree/feat/iommu).

| Platform | ${PLAT} |
| - | - |
| *CVA6* | `cva6` |

:information_source: Originally, the hypervisor extension test framework supported multiple platforms (QEMU, Rocket, CVA6). However, to the best of our knowledge, only the CVA6-based platform has support for the RISC-V IOMMU. We keep the platform definition mechanism to enable the integration of support for other platforms through further contributions.

### Output level

The output level can be specified via the `LOG_LEVEL` environment variable (default is `LOG_INFO`). 

Options include:
`LOG_NONE`, `LOG_ERROR`, `LOG_INFO`, `LOG_DETAIL`, `LOG_WARNING`, `LOG_VERBOSE`, `LOG_DEBUG`

### Compile

The build process is as follows:

```bash
$ git clone https://github.com/zero-day-labs/riscv-iommu-tests
$ cd riscv-iommu-tests
$ make PLAT=${target_platform}
```

The output files are located in the **build/`${PLAT}`** folder (**rv_iommu_test.elf** and **rv_iommu_test.bin**).
