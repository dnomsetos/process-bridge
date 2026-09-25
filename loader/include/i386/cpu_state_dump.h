#pragma once

#include <linux/i386/process_state.h>

#include <stdint.h>

#include <unicorn/unicorn.h>

uint64_t dump_i386_cpu_state(uc_engine *engine,
                             linux_i386_process_state_t *state);
