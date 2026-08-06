#pragma once

#include <linux/arch.h>

void dump_snapshot(snapshot_info_t *snapshot, pid_t pid, const char *filename);
