#pragma once

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

static inline int ptrace_traceme_is_usable(void) {
  pid_t pid = fork();
  if (pid < 0) {
    return 0;
  }

  if (pid == 0) {
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
      _exit(1);
    }
    _exit(0);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    return 0;
  }

  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
