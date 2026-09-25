#pragma once

#include <linux/i386/process_state.h>
#include <linux/x86_64/process_state.h>

#include <unicorn/unicorn.h>

typedef struct {
  uc_arch arch;
  uc_mode mode;

  union {
    linux_x86_64_process_state_t x86_64;
    linux_i386_process_state_t i386;
  };
} cpu_state_t;
