#pragma once

#include <asm/ldt.h>
#include <sys/user.h>

#include <linux/breakpoint.h>

typedef struct {
  uint32_t base_addr;
  uint32_t limit;
  uint32_t flags;
} tls_segment_t;

typedef struct {
  struct user_regs_struct regs;
  tls_segment_t seg;
} snapshot_info_t;

void create_snapshot(snapshot_info_t *snapshot, breakpoint_t *breakpoint);
