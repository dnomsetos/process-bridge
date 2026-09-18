#pragma once

#include <asm/ldt.h>
#include <sys/user.h>

#include <linux/breakpoint.h>

typedef struct {
  struct user_regs_struct regs;
  struct user_desc tls[3];
} snapshot_info_t;

void create_snapshot(snapshot_info_t *snapshot, const breakpoint_t *breakpoint);
