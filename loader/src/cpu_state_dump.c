#include <cpu_state_dump.h>

uint64_t dump_cpu_state(uc_engine *engine, cpu_state_t *state) {
  switch (state->mode) {
    case UC_MODE_32:
      return dump_i386_cpu_state(engine, &state->i386);
    case UC_MODE_64:
      return dump_x86_64_cpu_state(engine, &state->x86_64);
    default:
      return -1;
  }
}
