#include <dump.h>
#include <loader.h>
#include <log.h>
#include <uc_utils.h>

#include <unicorn/unicorn.h>

#define PB_EXPORT \
  __attribute__((visibility("default"))) __attribute__((noinline))

typedef struct {
  uc_arch arch;
  uc_mode mode;
} arch_mode_t;

static arch_mode_t supported_arch_modes[] = {
    {UC_ARCH_X86, UC_MODE_32},
    {UC_ARCH_X86, UC_MODE_64},
};

PB_EXPORT uint64_t pb_create_from_snapshot(uc_arch arch, uc_mode mode,
                                           uc_engine **engine,
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

  PB_UC_CHECK(uc_open(arch, mode, engine), "uc_open failed: %s");
  LOG_INFO("initializing snapshot for uc_arch %d uc_mode %d", arch, mode);

  PB_UC_CHECK(uc_open(arch, mode, engine), "uc_open failed: %s");

  return pb_restore_snapshot(*engine, snapshot_path);
}

PB_EXPORT uint64_t pb_restore_snapshot(uc_engine *uc,
                                       const char *snapshot_path) {
  uint64_t entry_point = dump_snapshot(uc, snapshot_path);

  if (entry_point == -1) {
    LOG_FATAL("failed to dump snapshot");
    return -1;
  }

  return entry_point;
}
