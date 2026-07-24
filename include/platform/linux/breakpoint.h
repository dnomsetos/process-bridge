#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
  pid_t pid;
  void *hook_addr;
  size_t old_word;
} breakpoint_t;

int fork_child(const char *path, char *const *argv, char *const *envp);

void create_breakpoint(breakpoint_t *breakpoint, pid_t pid, void *hook_addr);

void execute_up_to_breakpoint(breakpoint_t *breakpoint);

void create_snapshot(breakpoint_t *breakpoint);
