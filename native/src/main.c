#include <complex.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <log.h>
#include <platform.h>

extern char **environ;

int main(int argc, char *argv[]) {
  log_init();

  if (argc < 3) {
    fprintf(stderr,
            "Usage: %s <breakpoint_offset_hex> <child_path> [child_args...]\n",
            argc > 0 ? argv[0] : "process-bridge");
    return EXIT_FAILURE;
  }

  char *end;
  errno = 0;
  uintptr_t addr = strtoull(argv[1], &end, 16);

  if (errno != 0 || *end != '\0' || end == argv[1]) {
    LOG_FATAL("invalid breakpoint offset '%s': expected a hex number", argv[1]);
  }

  char *child = argv[2];

  int extra_argc = argc - 3;
  char **child_argv = malloc((size_t)(extra_argc + 2) * sizeof(char *));
  if (child_argv == NULL) {
    LOG_FATAL_ERRNO("malloc for child argv failed");
  }

  child_argv[0] = child;
  for (int i = 0; i < extra_argc; ++i) {
    child_argv[1 + i] = argv[3 + i];
  }
  child_argv[1 + extra_argc] = NULL;

  char *empty_envp[] = {NULL};
  char **child_envp = environ ? environ : empty_envp;

  pid_t pid = fork_child(child, child_argv, child_envp);
  free(child_argv);

  uintptr_t image_base = get_image_base(child, pid);

  LOG_INFO("image base: 0x%" PRIxPTR, image_base);

  breakpoint_t breakpoint;
  create_breakpoint(&breakpoint, pid, (void *)(image_base + addr));

  execute_up_to_breakpoint(&breakpoint);

  snapshot_info_t snapshot;

  create_snapshot(&snapshot, &breakpoint);
  dump_snapshot(&snapshot, pid, "ql_snapshot");

  if (kill(pid, SIGKILL) == -1 && errno != ESRCH) {
    LOG_ERRNO(LOG_LEVEL_WARN, "kill(%d, SIGKILL) failed", pid);
  }

  if (waitpid(pid, NULL, 0) == -1 && errno != ECHILD) {
    LOG_ERRNO(LOG_LEVEL_WARN, "waitpid(%d) after kill failed", pid);
  }

  return EXIT_SUCCESS;
}
