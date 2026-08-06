#include <complex.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <platform.h>

extern char **environ;

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Invalid usage");
    exit(EXIT_FAILURE);
  }

  char *end;
  uintptr_t addr = strtoull(argv[1], &end, 16);

  if (errno != 0 || *end != '\0') {
    perror("strtoull");
    exit(EXIT_FAILURE);
  }

  char *child = argv[2];

  char *empv[] = {NULL};

  char **child_argv = argv[3] ? &argv[3] : empv;

  pid_t pid = fork_child(child, child_argv, environ ? environ : empv);

  uintptr_t image_base = get_image_base(child, pid);

  printf("image base: %" PRIxPTR "\n", image_base);

  breakpoint_t breakpoint;
  create_breakpoint(&breakpoint, pid, (void *)(image_base + addr));

  execute_up_to_breakpoint(&breakpoint);

  snapshot_info_t snapshot;

  create_snapshot(&snapshot, &breakpoint);
  dump_snapshot(&snapshot, pid, "ql_snapshot");

  kill(pid, SIGKILL);
  waitpid(pid, NULL, 0);

  return 0;
}
