#ifndef _INCLUDE_PGT_H_
#define _INCLUDE_PGT_H_

#define PGT_VALID_MASK 0x1
#define PGT_ADDRESS_MASK 0xFFFFFFFFFF000
#define PGT_SIZE 4096
#define PGT_HUGE_MASK 0x80
#define PGT_WRITABLE_MASK 0x2
#define PGT_USER_MASK 0x4

#define PGT_IS_VALID(p) (p & PGT_VALID_MASK)
#define PGT_IS_HUGEPAGE(p) (p & PGT_HUGE_MASK)
#define PGT_ADDRESS(p) (p & PGT_ADDRESS_MASK)

#define PGT_PLM4_INDEX_MASK 0xFF8000000000
#define PGT_PLM3_INDEX_MASK 0x7FC0000000
#define PGT_PLM2_INDEX_MASK 0x3FE00000
#define PGT_PLM1_INDEX_MASK 0x1FF000

#define USER_STACK_BEGIN 0x40000000
#define USER_STACK_END 0x2000000000

#define PGT_PLM4_INDEX(v) ((v & PGT_PLM4_INDEX_MASK) >> 39)
#define PGT_PLM3_INDEX(v) ((v & PGT_PLM3_INDEX_MASK) >> 30)
#define PGT_PLM2_INDEX(v) ((v & PGT_PLM2_INDEX_MASK) >> 21)
#define PGT_PLM1_INDEX(v) ((v & PGT_PLM1_INDEX_MASK) >> 12)

#endif