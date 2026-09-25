#pragma once

#include <stdint.h>

#define PB_GDT_ADDR (uint64_t)0x3000
#define PB_GDT_SIZE (uint64_t)0x1000

#define _DESC_ACCESSED         0x0001
#define _DESC_DATA_WRITABLE    0x0002
#define _DESC_CODE_READABLE    0x0002
#define _DESC_DATA_EXPAND_DOWN 0x0004
#define _DESC_CODE_CONFORMING  0x0004
#define _DESC_CODE_EXECUTABLE  0x0008

#define _DESC_S        0x0010
#define _DESC_DPL(dpl) ((dpl) << 5)
#define _DESC_PRESENT  0x0080

#define _DESC_LONG_CODE      0x2000
#define _DESC_DB             0x4000
#define _DESC_GRANULARITY_4K 0x8000

#define _DESC_DATA \
  (_DESC_S | _DESC_PRESENT | _DESC_ACCESSED | _DESC_DATA_WRITABLE)
#define _DESC_CODE                                                  \
  (_DESC_S | _DESC_PRESENT | _DESC_ACCESSED | _DESC_CODE_READABLE | \
   _DESC_CODE_EXECUTABLE)

#define DESC_DATA32 (_DESC_DATA | _DESC_GRANULARITY_4K | _DESC_DB)
#define DESC_CODE32 (_DESC_CODE | _DESC_GRANULARITY_4K | _DESC_DB)

#define DESC_DATA64 (_DESC_DATA | _DESC_GRANULARITY_4K | _DESC_DB)
#define DESC_CODE64 (_DESC_CODE | _DESC_GRANULARITY_4K | _DESC_LONG_CODE)

#define DESC_USER (_DESC_DPL(3))

#define GDT_ENTRY_INIT(flags, base, limit)    \
  ((((uint64_t)(limit) & 0xFFFFULL) << 0) |   \
   (((uint64_t)(base) & 0xFFFFULL) << 16) |   \
   (((uint64_t)(base) & 0xFF0000ULL) << 16) | \
   (((uint64_t)(flags) & 0x000FULL) << 40) |  \
   (((uint64_t)(flags) & 0x0010ULL) << 40) |  \
   (((uint64_t)(flags) & 0x0060ULL) << 40) |  \
   (((uint64_t)(flags) & 0x0080ULL) << 40) |  \
   (((uint64_t)(limit) & 0xF0000ULL) << 32) | \
   (((uint64_t)(flags) & 0x1000ULL) << 40) |  \
   (((uint64_t)(flags) & 0x2000ULL) << 40) |  \
   (((uint64_t)(flags) & 0x4000ULL) << 40) |  \
   (((uint64_t)(flags) & 0x8000ULL) << 40) |  \
   (((uint64_t)(base) & 0xFF000000ULL) << 32))

#define USER_DESC_TO_REAL_DESC(user_desc)                     \
  ((((uint64_t)((user_desc).limit & 0xFFFF)) << 0) |          \
   (((uint64_t)((user_desc).base_addr & 0xFFFF)) << 16) |     \
   (((uint64_t)((user_desc).base_addr >> 16) & 0xFF) << 32) | \
   (((uint64_t)(((user_desc).read_exec_only ^ 1) << 1) |      \
     ((user_desc).contents << 2) | 1)                         \
    << 40) |                                                  \
   ((uint64_t)1 << 44) | ((uint64_t)3 << 45) |                \
   ((uint64_t)((user_desc).seg_not_present ^ 1) << 47) |      \
   (((uint64_t)((user_desc).limit >> 16) & 0xF) << 48) |      \
   ((uint64_t)(user_desc).useable << 52) |                    \
   ((uint64_t)(user_desc).seg_32bit << 54) |                  \
   ((uint64_t)(user_desc).limit_in_pages << 55) |             \
   (((uint64_t)((user_desc).base_addr >> 24) & 0xFF) << 56))
