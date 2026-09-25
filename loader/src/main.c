#include <loader.h>

#include <unicorn/unicorn.h>

int main() {
  uc_engine *engine = NULL;
  uint64_t entry_point =
      pb_from_snapshot(UC_ARCH_X86, UC_MODE_64, &engine, "ql_snapshot");

  if (entry_point == -1) {
    return EXIT_FAILURE;
  }

  uc_emu_start(engine, entry_point, 0, 0, 0);

  uc_close(engine);
  return 0;
}
