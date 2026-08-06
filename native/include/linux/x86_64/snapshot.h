#pragma once

#include <sys/user.h>

#include <linux/breakpoint.h>

typedef struct {
  struct user_regs_struct regs;
} snapshot_info_t;

void create_snapshot(snapshot_info_t *snapshot, breakpoint_t *breakpoint);
