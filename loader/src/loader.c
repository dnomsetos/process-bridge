#include <dump.h>
#include <loader.h>
#include <log.h>
#include <uc_utils.h>

#include <unicorn/unicorn.h>

typedef struct {
  uc_arch arch;
  uc_mode mode;
} arch_mode_t;

static arch_mode_t supported_arch_modes[] = {
    {UC_ARCH_X86, UC_MODE_32},
    {UC_ARCH_X86, UC_MODE_64},
};

uint64_t pb_from_snapshot(uc_arch arch, uc_mode mode, uc_engine **engine,
                          const char *snapshot_path) {
  arch_mode_t arch_mode = {.arch = arch, .mode = mode};

  bool supported = false;
  for (int i = 0; i < sizeof(supported_arch_modes) / sizeof(arch_mode_t); ++i) {
    if (arch_mode.arch == supported_arch_modes[i].arch &&
        arch_mode.mode == supported_arch_modes[i].mode) {
      supported = true;
      break;
    }
  }

  if (!supported) {
    LOG_ERROR("unsupported uc_arch %d uc_mode %d", arch, mode);
    *engine = NULL;
    return -1;
  }

  LOG_INFO("initializing snapshot for uc_arch %d uc_mode %d", arch, mode);

  PB_UC_CHECK(uc_open(arch, mode, engine), "uc_open failed: %s");

  uint64_t entry_point = dump_snapshot(*engine, snapshot_path);

  if (entry_point == -1) {
    LOG_FATAL("failed to dump snapshot");
    return -1;
  }

  return entry_point;
}
