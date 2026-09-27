#pragma once

#include <i386/cpu_state_dump.h>
#include <snapshot_variant.h>
#include <x86_64/cpu_state_dump.h>

uint64_t dump_cpu_state(uc_engine *engine, cpu_state_t *state);
