#pragma once

#include <linux/x86_64/process_state.h>

#include <stdint.h>

#include <unicorn/unicorn.h>

uint64_t dump_x86_64_cpu_state(uc_engine *engine,
                               linux_x86_64_process_state_t *state);
