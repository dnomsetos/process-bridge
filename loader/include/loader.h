#pragma once

#include <stdint.h>

#include <unicorn/unicorn.h>

uint64_t pb_from_snapshot(uc_arch arch, uc_mode mode, uc_engine **engine,
                          const char *snapshot_path);
