#pragma once

#include <linux/arch.h>

void dump_snapshot(process_state_t *snapshot, pid_t pid, const char *filename);
