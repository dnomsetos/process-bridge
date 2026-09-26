#include <loader.h>
#include <log.h>

#include <sys/syscall.h>

#include <unicorn/unicorn.h>

void hook_code(uc_engine *uc, uint64_t address, uint32_t size,
               void *user_data) {
  fprintf(stderr, "rip=0x%lx size=%u\n", address, size);
}

int main() {
  uc_engine *uc = NULL;
  uint64_t entry_point =
      pb_from_snapshot(UC_ARCH_X86, UC_MODE_64, &uc, "ql_snapshot");

  if (entry_point == -1) {
    return EXIT_FAILURE;
  }

  uc_hook trace;
  uc_hook_add(uc, &trace, UC_HOOK_CODE, hook_code, NULL, 1, 0);

  LOG_DEBUG("entry point: %lx", entry_point);

  uc_err err = uc_emu_start(uc, entry_point, 0, 0, 0);
  if (err != UC_ERR_OK) {
    LOG_FATAL("failed to start emulation: %s", uc_strerror(err));
    return 1;
  }

  uc_close(uc);

  LOG_INFO("success");
  return 0;
}
