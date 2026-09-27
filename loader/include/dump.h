#pragma once

#include <stdint.h>

#include <unicorn/unicorn.h>

uint64_t dump_snapshot(uc_engine *engine, const char *snapshot_path);
