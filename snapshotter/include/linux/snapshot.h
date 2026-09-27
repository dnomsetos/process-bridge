#pragma once

#include <asm/ldt.h>
#include <sys/user.h>

#include <linux/arch.h>
#include <linux/breakpoint.h>

void create_snapshot(process_state_t *snapshot, const breakpoint_t *breakpoint);
