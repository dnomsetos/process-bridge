#pragma once

#include <asm/ldt.h>
#include <sys/user.h>

#include <linux/breakpoint.h>

typedef struct {
  struct user_regs_struct regs;
  struct user_desc tls[3];
} process_state_t;

void create_snapshot(process_state_t *snapshot, const breakpoint_t *breakpoint);
