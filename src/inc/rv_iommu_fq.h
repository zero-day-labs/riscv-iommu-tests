#ifndef FAULT_QUEUE_H
#define FAULT_QUEUE_H

// Mask for cqb.PPN (fqb[53:10])
#define FQB_PPN_MASK    (0x3FFFFFFFFFFC00ULL)
// Mask for CQ PPN (cqb[55:12])
#define FQ_PPN_MASK     (0xFFFFFFFFFFF000ULL)

#define CAUSE_MASK      (0xFFFULL)
#define TTYP_MASK       (0xFC00000000ULL)
#define DID_MASK        (0xFFFFFF0000000000ULL)
#define DID_OFF         (40)
// Offset for fqb.PPN
#define FQB_PPN_OFF     (10)

// fqcsr masks
#define FQCSR_FQEN      (1ULL << 0 )
#define FQCSR_FIE       (1ULL << 1 )
#define FQCSR_FQMF      (1ULL << 8 )
#define FQCSR_FQOF      (1ULL << 9 )
#define FQCSR_FQON      (1ULL << 16)
#define FQCSR_BUSY      (1ULL << 17)

// Fault encoding
#define INSTR_ACCESS_FAULT     (1 )
#define LD_ADDR_MISALIGNED     (4 )
#define LD_ACCESS_FAULT        (5 )
#define ST_ADDR_MISALIGNED     (6 )
#define ST_ACCESS_FAULT        (7 )
#define INSTR_PAGE_FAULT       (12)
#define LOAD_PAGE_FAULT        (13)
#define STORE_PAGE_FAULT       (15)
#define INSTR_GUEST_PAGE_FAULT (20)
#define LOAD_GUEST_PAGE_FAULT  (21)
#define STORE_GUEST_PAGE_FAULT (23)

#define ALL_INB_TRANSACTIONS_DISALLOWED     (256)
#define DDT_ENTRY_LD_ACCESS_FAULT           (257)
#define DDT_ENTRY_INVALID                   (258)
#define DDT_ENTRY_MISCONFIGURED             (259)
#define TRANS_TYPE_DISALLOWED               (260)
#define MSI_PTE_LD_ACCESS_FAULT             (261)
#define MSI_PTE_INVALID                     (262)
#define MSI_PTE_MISCONFIGURED               (263)
#define MRIF_ACCESS_FAULT                   (264)
#define PDT_ENTRY_LD_ACCESS_FAULT           (265)
#define PDT_ENTRY_INVALID                   (266)
#define PDT_ENTRY_MISCONFIGURED             (267)
#define DDT_DATA_CORRUPTION                 (268)
#define PDT_DATA_CORRUPTION                 (269)
#define MSI_PT_DATA_CORRUPTION              (270)
#define MSI_MRIF_DATA_CORRUPTION            (271)
#define INTERN_DATAPATH_FAULT               (272)
#define MSI_ST_ACCESS_FAULT                 (273)
#define PT_DATA_CORRUPTION                  (274)

#endif  /* FAULT_QUEUE_H */