#pragma once

#include <asm/ldt.h>
#include <stdint.h>
#include <sys/user.h>

#define GDT_ENTRY_TLS_ENTRIES 3

struct pb_i386_regs {
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
  uint32_t esi;
  uint32_t edi;
  uint32_t ebp;
  uint32_t eax;
  uint32_t xds;
  uint32_t xes;
  uint32_t xfs;
  uint32_t xgs;
  uint32_t orig_eax;
  uint32_t eip;
  uint32_t xcs;
  uint32_t eflags;
  uint32_t esp;
  uint32_t xss;
};

typedef struct {
  struct pb_i386_regs regs;
  struct user_desc tls[GDT_ENTRY_TLS_ENTRIES];
} linux_i386_process_state_t;
