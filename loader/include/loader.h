#pragma once

#include <stdint.h>

#include <unicorn/unicorn.h>

#define PB_EXPORT \
  __attribute__((visibility("default"))) __attribute__((noinline))

PB_EXPORT
uint64_t pb_create_from_snapshot(uc_arch arch, uc_mode mode, uc_engine **engine,
                                 const char *snapshot_path);
PB_EXPORT
uint64_t pb_restore_snapshot(uc_engine *uc, const char *snapshot_path);
